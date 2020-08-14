#include "SDL2/SDL.h"
#include <deque>
#include <thread>
#include <mutex>
#include <chrono>
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <libswresample/swresample.h>
}
static Uint8  *audio_chunk;
static Uint32  audio_len;
static Uint8  *audio_pos;
static void fill_audio(void *udata,Uint8 *stream,int len)
{
    //SDL 2.0
    SDL_memset(stream, 0, len);
    if(audio_len==0)
        return;
    len=(len>(int)audio_len?(int)audio_len:len);	/*  Mix  as  much  data  as  possible  */
    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}
class IMedia
{
public:
    virtual void init(const char *url) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
};
class CFFmpeg_Audio : public IMedia
{
    AVFormatContext	*pFormatCtx{};
    int				i{}, audioStream{};
    AVCodecContext	*pCodecCtx{};
    AVCodec			*pCodec{};
    AVPacket		*packet{};
    uint8_t			*out_buffer{};
    AVFrame			*pFrame{};
    SDL_AudioSpec wanted_spec{};
    int ret{};
//    uint32_t len;
    int got_picture{};
    int index{};
    int64_t in_channel_layout{};
    struct SwrContext *au_convert_ctx{};
    int out_buffer_size{};
    std::mutex mu;
    std::deque<AVFrame*> aframe_buf;
    bool flag{};
    AVCodecContext *pCodecCtxOrig{};
    AVCodecParameters *pParam{};
    std::thread thread1;
    std::thread thread2;

    void init_audio(const char *url)
    {
        flag = true;
        if(avformat_open_input(&pFormatCtx, url, nullptr, nullptr)!=0)//argv[1]
        {
            printf("Couldn't open file\n");
            return; // Couldn't open file
        }

        if(avformat_find_stream_info(pFormatCtx, nullptr)<0)
        {
            printf("Couldn't find stream information\n");
            return; // Couldn't find stream information
        }

        av_dump_format(pFormatCtx, 0, url, 0);
    }
    void init_contexts()
    {
        audioStream = -1;
        for (i = 0; i < pFormatCtx->nb_streams; i++)
            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                audioStream = i;
                break;
            }
        if (audioStream == -1)
            return;

        pCodecCtxOrig = pFormatCtx->streams[audioStream]->codec;
        pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
        if (pCodec == nullptr) {
            fprintf(stderr, "Unsupported codec!\n");
            return; // Codec not found
        }
        pCodecCtx = avcodec_alloc_context3(pCodec);
        pParam = avcodec_parameters_alloc();
        avcodec_parameters_from_context(pParam, pCodecCtxOrig);
        avcodec_parameters_to_context(pCodecCtx, pParam);
        avcodec_parameters_free(&pParam);
        if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
            return;

        packet=(AVPacket *)av_malloc(sizeof(AVPacket));////////////////////////OPTIONAL
        av_init_packet(packet);////////////////////////////////////////////////OPTIONAL

//    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);
        out_buffer_size = av_samples_get_buffer_size(nullptr, 2, pCodecCtx->frame_size, AV_SAMPLE_FMT_S16, 1);//AV_SAMPLE_FMT_S16


        out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
        pFrame=av_frame_alloc();

        //FIX:Some Codec's Context Information is missing
        in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
    }
    void init_SDL()
    {
        if(SDL_Init(SDL_INIT_AUDIO))
        {
            printf( "Could not initialize SDL - %s\n", SDL_GetError());
            return;
        }
        //SDL_AudioSpec
        wanted_spec.freq = pCodecCtx->sample_rate;
        wanted_spec.format = AUDIO_S16SYS;//AUDIO_S16SYS
        wanted_spec.channels = pCodecCtx->channels;
//    wanted_spec.silence = 0;
        wanted_spec.samples = pCodecCtx->frame_size;
        wanted_spec.callback = fill_audio;
        wanted_spec.userdata = pCodecCtx;

        if (SDL_OpenAudio(&wanted_spec, nullptr)<0){
            printf("can't open audio.\n");
            exit(1);
        }
        au_convert_ctx = swr_alloc();
        au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, pCodecCtx->sample_rate,
                                          in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, nullptr);
        swr_init(au_convert_ctx);
    }
    void write_frames()
    {
        AVRational a = {1,1000};
        while (av_read_frame(pFormatCtx, packet) >= 0)
        {
            if (packet->stream_index == audioStream)
            {
                pFrame=av_frame_alloc();
                avcodec_send_packet(pCodecCtx, packet);
                avcodec_receive_frame(pCodecCtx, pFrame);
                pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, a);
                while (aframe_buf.size() > 100)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                mu.lock();
                aframe_buf.push_back(pFrame);
                mu.unlock();
            }
            av_packet_unref(packet);
        }
        flag = false;
    }
    void read_frames()
    {
        SDL_PauseAudio(0);

        while (true)
        {
            mu.lock();
            if (aframe_buf.empty() && !flag)
            {
                mu.unlock();
                SDL_CloseAudio();//Close SDL
                SDL_Quit();
                return;
            }
            if (aframe_buf.empty())
            {
                mu.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            AVFrame *frame;
            frame = aframe_buf.front();
            aframe_buf.pop_front();
            mu.unlock();
            swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);

            while(audio_len>0)//Wait until finish
                SDL_Delay(1);

            audio_chunk = (Uint8 *) out_buffer;
            audio_len = out_buffer_size;
            audio_pos = audio_chunk;
            av_frame_unref(frame);
        }

        SDL_CloseAudio();//Close SDL
        SDL_Quit();
    }
public:
    CFFmpeg_Audio() = default;

    void init(const char *url) override
    {
        init_audio(url);
        init_contexts();
        init_SDL();
    }
    void play() override
    {
//        write_frames();
//        read_frames();

        thread1 = std::thread(&CFFmpeg_Audio::write_frames, this);
        thread2 = std::thread(&CFFmpeg_Audio::read_frames, this);


//        SDL_PauseAudio(0);
//        while (av_read_frame(pFormatCtx, packet) >= 0)
//        {
//            if (packet->stream_index == audioStream)
//            {
////            pFrame=av_frame_alloc();
//                avcodec_send_packet(pCodecCtx, packet);
//                avcodec_receive_frame(pCodecCtx, pFrame);
//
//                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
//                pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, {1, 1000});
//                printf("index:%5d\t pts:%ld\t packet size:%d\n",index,pFrame->pts,packet->size);
//
//                index++;
//
//
//                while(audio_len>0)//Wait until finish
//                    SDL_Delay(1);
//
////            //Set audio buffer (PCM data)
//                audio_chunk = (Uint8 *) out_buffer;
////            //Audio buffer length
//                audio_len = out_buffer_size;
//                audio_pos = audio_chunk;
//            }
//            av_packet_unref(packet);
//        }
    }
    void stop() override
    {
        thread1.join();
        thread2.join();
        swr_free(&au_convert_ctx);
        av_free(out_buffer);
        avcodec_close(pCodecCtxOrig);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);
    }
};
class CFFmpegVideo : public IMedia
{
    AVFormatContext *pFormatCtx{};
    int i{},videoStream{};
    AVCodecContext *pCodecCtxOrig{};
    AVCodecContext *pCodecCtx{};
    AVCodec *pCodec{};
    AVCodecParameters *pParam{};
    AVFrame *pFrame{};
//    uint8_t *buffer;
    struct SwsContext *sws_ctx{};
//    int frameFinished{};
    AVPacket *packet{};
    SDL_Window *window{};
    SDL_Event event{};
    SDL_Renderer *renderer{};
    SDL_Texture *texture{};
    Uint8 *yPlane{}, *uPlane{}, *vPlane{};
    size_t yPlaneSz{}, uvPlaneSz{};
    int uvPitch{};
    std::mutex mu;
    std::deque<AVFrame*> vframe_buf;
    bool flag{};
    std::thread thread1;
    std::thread thread2;

    void init_video(const char *url)
    {
        if(avformat_open_input(&pFormatCtx, url, nullptr, nullptr)!=0)//argv[1]
        {
            printf("Couldn't open file\n");
            return; // Couldn't open file
        }

        if(avformat_find_stream_info(pFormatCtx, nullptr)<0)
        {
            printf("Couldn't find stream information\n");
            return; // Couldn't find stream information
        }

        av_dump_format(pFormatCtx, 0, url, 0);
    }
    void init_contexts()
    {
        videoStream=-1;
        for(i=0; i<pFormatCtx->nb_streams; i++)
            if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
            {
                videoStream=i;
                break;
            }
        if(videoStream==-1)
        {
            printf("this is not a video");
            return;
        }


        pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
        pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
        if(pCodec==nullptr)
        {
            fprintf(stderr, "Unsupported codec!\n");
            return; // Codec not found
        }
        pCodecCtx = avcodec_alloc_context3(pCodec);
        pParam = avcodec_parameters_alloc();
        avcodec_parameters_from_context(pParam, pCodecCtxOrig);
        avcodec_parameters_to_context(pCodecCtx, pParam);
        avcodec_parameters_free(&pParam);
        if(avcodec_open2(pCodecCtx, pCodec, nullptr)<0)
            return;

        sws_ctx = sws_getContext(pCodecCtx->width,
                                 pCodecCtx->height,
                                 pCodecCtx->pix_fmt,
                                 pCodecCtx->width,
                                 pCodecCtx->height,
                                 AV_PIX_FMT_YUV420P,
                                 SWS_BILINEAR,
                                 nullptr,
                                 nullptr,
                                 nullptr
        );
    }
    void init_SDL()
    {
        if(videoStream==-1)
            return;
        flag = true;
        window = SDL_CreateWindow(
                "SDL2Test",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                pCodecCtx->width,
                pCodecCtx->height,
                0
        );

        renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer) {
            fprintf(stderr, "SDL: could not create renderer - exiting\n");
            exit(1);
        }
        texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_YV12,
                SDL_TEXTUREACCESS_STREAMING,
                pCodecCtx->width,
                pCodecCtx->height
        );
        if (!texture) {
            fprintf(stderr, "SDL: could not create texture - exiting\n");
            exit(1);
        }
        // set up YV12 pixel array (12 bits per pixel)
        yPlaneSz = pCodecCtx->width * pCodecCtx->height;
        uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
        yPlane = (Uint8*)malloc(yPlaneSz);
        uPlane = (Uint8*)malloc(uvPlaneSz);
        vPlane = (Uint8*)malloc(uvPlaneSz);
        if (!yPlane || !uPlane || !vPlane) {
            fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
            exit(1);
        }
        uvPitch = pCodecCtx->width / 2;
    }
    void write_frames()
    {
        AVRational a = {1, 1000};
        packet = av_packet_alloc();
        while(av_read_frame(pFormatCtx, packet)>=0)
        {
            // Is this a packet from the video stream?
            if(packet->stream_index==videoStream)
            {
                pFrame=av_frame_alloc();
                avcodec_send_packet(pCodecCtx, packet);
                avcodec_receive_frame(pCodecCtx, pFrame);
                pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[videoStream]->time_base, a);

                while (vframe_buf.size() > 100)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
                mu.lock();
                vframe_buf.push_back(pFrame);
                mu.unlock();
            }
        }
        av_packet_unref(packet);
        flag = false;
    }
    void read_frames()
    {
        int PTS;
        long double msec;
        struct timespec time1, time2;
        clock_gettime(CLOCK_REALTIME, &time2);
        while (true)
        {
            AVFrame *frame;
            mu.lock();
            if (!flag && vframe_buf.empty())
            {
                SDL_DestroyTexture(texture);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                mu.unlock();
                return;
            }
            if(vframe_buf.empty())
            {
                mu.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            frame = vframe_buf.front();
            vframe_buf.pop_front();
            mu.unlock();

            PTS = frame->pts;
            while (true) {
                clock_gettime(CLOCK_REALTIME, &time1);
                msec = 1000 * (time1.tv_sec - time2.tv_sec) + (time1.tv_nsec - time2.tv_nsec) / 1000000;
                if (PTS <= msec)
                    break;
            }
            printf("Time from start video = %Lf\n", msec);
            AVPicture pict;
            pict.data[0] = yPlane;
            pict.data[1] = uPlane;
            pict.data[2] = vPlane;
            pict.linesize[0] = pCodecCtx->width;
            pict.linesize[1] = uvPitch;
            pict.linesize[2] = uvPitch;
//                // Convert the image into YUV format that SDL uses
            sws_scale(sws_ctx, (uint8_t const *const *) frame->data, frame->linesize, 0, pCodecCtx->height, pict.data,
                      pict.linesize);
            SDL_UpdateYUVTexture(
                    texture,
                    nullptr,
                    yPlane,
                    pCodecCtx->width,
                    uPlane,
                    uvPitch,
                    vPlane,
                    uvPitch
            );
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            printf("Current PTS = %d\n", PTS);
            av_frame_unref(frame);
            SDL_PollEvent(&event);
            switch (event.type)
            {
                case SDL_QUIT:

                    SDL_DestroyTexture(texture);
                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(window);
                    SDL_Quit();
                    return;
                default:
                    break;
            }
        }
    }
public:
    CFFmpegVideo() = default;
    void init(const char *url) override
    {
        init_video(url);
        init_contexts();
        init_SDL();
    }
    void play() override
    {
        if(videoStream==-1)
            return;
        thread1 = std::thread(&CFFmpegVideo::write_frames, this);
        thread2 = std::thread(&CFFmpegVideo::read_frames, this);

    }
    void stop() override
    {
        thread1.join();
        thread2.join();
        free(yPlane);
        free(uPlane);
        free(vPlane);
        avcodec_close(pCodecCtxOrig);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);
    }
};

int main(int argc, char *argv[])
{
    IMedia *video = new CFFmpegVideo;
    IMedia *audio = new CFFmpeg_Audio;
    char *url = argv[1];
    video->init(url);
    audio->init(url);
//    std::thread video_play(&IVideo::play, video);
//    std::thread audio_play(&IAudio::play, audio);
//    video_play.join();
//    audio_play.join();
    video->play();
    audio->play();
    video->stop();
    audio->stop();
    return 0;
}

























