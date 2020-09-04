//#include "SDL2/SDL.h"
//#include <deque>
//#include <thread>
//#include <mutex>
//#include <chrono>
//#include <functional>
//#include <iostream>
//#include <fstream>
//#include <cstring>
//#include <linux/input.h>
//#include <atomic>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
////#include <linux/input-event-codes.h>
//#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
//
//extern "C"
//{
////#include "libavcodec/avcodec.h"//?
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
////#include "libavutil/imgutils.h"
//#include <libswresample/swresample.h>
//}
//
//static Uint8  *audio_chunk;
//static Uint32  audio_len;
//static Uint8  *audio_pos;
////AVRational a = {1, 1000};///substitution causes an error, why?
//attribute_deprecated bool VideoStop = false;
//
//class IMedia
//{
//public:
//    enum MediaStatus
//    {
//        NOT_INITIALIZED,
//        STARTED,
//        FINISHED
//    };
//
//    std::function<void()> Callback;
//    void SetCallback(std::function<void()> fn)
//    {
//        Callback = std::move(fn);
//    }
//
//protected:
//
//    MediaStatus status{};
//public:
////    getStatus();
//    virtual void init(const char *url) = 0;
//    attribute_deprecated virtual void play() = 0;
//    virtual void stop() = 0;
//    virtual int GetCurrentPTS() = 0;
//    virtual bool MediaFinished() = 0;
//    virtual int ReadFrame() = 0;
//    virtual void write_frames() = 0;
//    virtual ~IMedia()= default;
//    virtual MediaStatus GetStatus() = 0;
//    virtual void SetStatus(MediaStatus mediaStatus) = 0;
//};
//
//
//class CFFmpeg_Audio : public IMedia
//{
//    AVFormatContext	*pFormatCtx{};
//    int				i{}, audioStream{};
//    AVCodecContext	*pCodecCtx{};
//    AVCodec			*pCodec{};
//    AVPacket		*packet{};
//    uint8_t			*out_buffer{};
//    AVFrame			*pFrame{};
//    SDL_AudioSpec wanted_spec{};
//    int ret{};
////    uint32_t len;
//    int got_picture{};
//    int index{};
//    int64_t in_channel_layout{};
//    struct SwrContext *au_convert_ctx{};
//    int out_buffer_size{};
//    std::mutex mu;
//    std::deque<AVFrame*> aframe_buf;
//    bool flag{}, Afinish{};
//    AVCodecContext *pCodecCtxOrig{};
//    AVCodecParameters *pParam{};
//    std::thread thread1;
//    std::thread thread2;
//    int CurrentPTS{};
//
//    static void fill_audio(void *udata,Uint8 *stream,int len)
//    {
//        //SDL 2.0
//        SDL_memset(stream, 0, len);
//        if(audio_len==0)
//            return;
//        len=(len>(int)audio_len?(int)audio_len:len);	/*  Mix  as  much  data  as  possible  */
//        SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
//        audio_pos += len;
//        audio_len -= len;
//    }
//    void write_current_frame()
//    {
//        if (packet->stream_index == audioStream)
//        {
//            pFrame=av_frame_alloc();
//            avcodec_send_packet(pCodecCtx, packet);
//            avcodec_receive_frame(pCodecCtx, pFrame);
//            pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, {1,1000});
//            while (aframe_buf.size() > 200)
//            {
//                std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            }
//            mu.lock();
//            aframe_buf.push_back(pFrame);
//            mu.unlock();
//        }
//        av_packet_unref(packet);
//    }
//    attribute_deprecated void read_current_frame(AVFrame *frame)
//    {
//        CurrentPTS = frame->pts;
//        swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);
//
//        while(audio_len>0)//Wait until finish
//            SDL_Delay(1);
//
//        audio_chunk = (Uint8 *) out_buffer;
//        audio_len = out_buffer_size;
//        audio_pos = audio_chunk;
//    }
//    int ReadFrame() override
//    {
//        AVFrame *frame;
//        mu.lock();
//        if (aframe_buf.empty() && !flag)
//        {
//            mu.unlock();
////            Afinish = true;
//            Callback();
////            status = FINISHED;
//            return 2;
//        }
//        if (aframe_buf.empty())
//        {
//            mu.unlock();
//            return 1;
//        }
//
//        frame = aframe_buf.front();
//        aframe_buf.pop_front();
//        mu.unlock();
//        CurrentPTS = frame->pts;
//        swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);
//
//        while(audio_len>0)//Wait until finish
//            SDL_Delay(1);
//
//        audio_chunk = (Uint8 *) out_buffer;
//        audio_len = out_buffer_size;
//        audio_pos = audio_chunk;
//        av_frame_unref(frame);
////        Callback();
//        return 0;
//    }
//    void init_audio(const char *url)
//    {
//        status = NOT_INITIALIZED;
//        if(avformat_open_input(&pFormatCtx, url, nullptr, nullptr)!=0)//argv[1]
//        {
//            printf("Couldn't open file\n");
//            return; // Couldn't open file
//        }
//
//        if(avformat_find_stream_info(pFormatCtx, nullptr)<0)
//        {
//            printf("Couldn't find stream information\n");
//            return; // Couldn't find stream information
//        }
//
//        av_dump_format(pFormatCtx, 0, url, 0);
//    }
//    void init_contexts()
//    {
//        audioStream = -1;
//        for (i = 0; i < pFormatCtx->nb_streams; i++)
//            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
//                audioStream = i;
//                break;
//            }
//        if (audioStream == -1)
//            return;
//
//        pCodecCtxOrig = pFormatCtx->streams[audioStream]->codec;
//        pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
//        if (pCodec == nullptr) {
//            fprintf(stderr, "Unsupported codec!\n");
//            return; // Codec not found
//        }
//        pCodecCtx = avcodec_alloc_context3(pCodec);
//        pParam = avcodec_parameters_alloc();
//        avcodec_parameters_from_context(pParam, pCodecCtxOrig);
//        avcodec_parameters_to_context(pCodecCtx, pParam);
//        avcodec_parameters_free(&pParam);
//        if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
//            return;
//
//        packet=(AVPacket *)av_malloc(sizeof(AVPacket));////////////////////////OPTIONAL
//        av_init_packet(packet);////////////////////////////////////////////////OPTIONAL
//
////    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);
//        out_buffer_size = av_samples_get_buffer_size(nullptr, 2, pCodecCtx->frame_size, AV_SAMPLE_FMT_S16, 1);//AV_SAMPLE_FMT_S16
//
//
//        out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
//        pFrame=av_frame_alloc();
//
//        //FIX:Some Codec's Context Information is missing
//        in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
//    }
//    void init_SDL()
//    {
//        flag = true;
//        if(SDL_Init(SDL_INIT_AUDIO))
//        {
//            printf( "Could not initialize SDL - %s\n", SDL_GetError());
//            return;
//        }
//        //SDL_AudioSpec
//        wanted_spec.freq = pCodecCtx->sample_rate;
//        wanted_spec.format = AUDIO_S16SYS;//AUDIO_S16SYS
//        wanted_spec.channels = pCodecCtx->channels;
////    wanted_spec.silence = 0;
//        wanted_spec.samples = pCodecCtx->frame_size;
//        wanted_spec.callback = fill_audio;
//        wanted_spec.userdata = pCodecCtx;
//
//        if (SDL_OpenAudio(&wanted_spec, nullptr)<0){
//            printf("can't open audio.\n");
//            exit(1);
//        }
//        au_convert_ctx = swr_alloc();
//        au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, pCodecCtx->sample_rate,
//                                          in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, nullptr);
//        swr_init(au_convert_ctx);
//    }
//    void write_frames() override
//    {
//
//        while (av_read_frame(pFormatCtx, packet) >= 0)
//        {
//            write_current_frame();
//        }
//        flag = false;
//    }
//    attribute_deprecated void read_frames()
//    {
//        SDL_PauseAudio(0);
//
//        while (true)
//        {
//            mu.lock();
//            if (aframe_buf.empty() && !flag)
//            {
//                mu.unlock();
////                SDL_CloseAudio();//Close SDL
////                SDL_Quit();
//                break;
//            }
//            if (aframe_buf.empty())
//            {
//                mu.unlock();
//                std::this_thread::sleep_for(std::chrono::milliseconds(2));
//                continue;
//            }
//
//            AVFrame *frame;
//            frame = aframe_buf.front();
//            aframe_buf.pop_front();
//            mu.unlock();
//            read_current_frame(frame);
//            av_frame_unref(frame);
//        }
//        Afinish = true;
//        SDL_CloseAudio();//Close SDL
//        SDL_Quit();
//    }
//public:
//    CFFmpeg_Audio() = default;
//    int GetCurrentPTS() override
//    {
//        return CurrentPTS;
//    }
//    bool MediaFinished() override
//    {
//        return Afinish;
//    }
//    void init(const char *url) override
//    {
//        init_audio(url);
//        init_contexts();
//        init_SDL();
//    }
//    attribute_deprecated void play() override
//    {
////        write_frames();
////        read_frames();
//
//        thread1 = std::thread(&CFFmpeg_Audio::write_frames, this);
//        thread2 = std::thread(&CFFmpeg_Audio::read_frames, this);
//
//
////        SDL_PauseAudio(0);
////        while (av_read_frame(pFormatCtx, packet) >= 0)
////        {
////            if (packet->stream_index == audioStream)
////            {
//////            pFrame=av_frame_alloc();
////                avcodec_send_packet(pCodecCtx, packet);
////                avcodec_receive_frame(pCodecCtx, pFrame);
////
////                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
////                pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, {1, 1000});
////                printf("index:%5d\t pts:%ld\t packet size:%d\n",index,pFrame->pts,packet->size);
////
////                index++;
////
////
////                while(audio_len>0)//Wait until finish
////                    SDL_Delay(1);
////
//////            //Set audio buffer (PCM data)
////                audio_chunk = (Uint8 *) out_buffer;
//////            //Audio buffer length
////                audio_len = out_buffer_size;
////                audio_pos = audio_chunk;
////            }
////            av_packet_unref(packet);
////        }
//    }
//    void stop() override
//    {
////        thread1.join();
////        thread2.join();
//        swr_free(&au_convert_ctx);
//        av_free(out_buffer);
//        avcodec_close(pCodecCtxOrig);
//        avcodec_close(pCodecCtx);
//        avformat_close_input(&pFormatCtx);
//    }
//    MediaStatus GetStatus() override
//    {
//        return status;
//    }
//    void SetStatus(MediaStatus status) override
//    {
//        this->status = status;
//    }
//};
//
//class CFFmpegVideo : public IMedia
//{
//    AVFormatContext *pFormatCtx{};
//    int i{},videoStream{};
//    AVCodecContext *pCodecCtxOrig{};
//    AVCodecContext *pCodecCtx{};
//    AVCodec *pCodec{};
//    AVCodecParameters *pParam{};
//    AVFrame *pFrame{};
////    uint8_t *buffer;
//    struct SwsContext *sws_ctx{};
////    int frameFinished{};
//    AVPacket *packet{};
//    SDL_Window *window{};
//    SDL_Event event{};
//    SDL_Renderer *renderer{};
//    SDL_Texture *texture{};
//    Uint8 *yPlane{}, *uPlane{}, *vPlane{};
//    size_t yPlaneSz{}, uvPlaneSz{};
//    int uvPitch{};
//    std::mutex mu;
//    std::deque<AVFrame*> vframe_buf;
//    bool flag{}, Vfinish{};
//    std::thread thread1{};
//    std::thread thread2{};
//    int CurrentPTS{};
//
//    void write_current_frame()
//    {
//        pFrame=av_frame_alloc();
//        avcodec_send_packet(pCodecCtx, packet);
//        avcodec_receive_frame(pCodecCtx, pFrame);
//        pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[videoStream]->time_base, {1,1000});
//
//        while (vframe_buf.size() > 100)
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds(2));
//        }
//        mu.lock();
//        vframe_buf.push_back(pFrame);
//        mu.unlock();
//    }
//    attribute_deprecated void read_current_frame(AVFrame *frame, AVPicture pict)
//    {
//        sws_scale(sws_ctx, (uint8_t const *const *) frame->data, frame->linesize, 0, pCodecCtx->height, pict.data,
//                  pict.linesize);
//        SDL_UpdateYUVTexture(
//                texture,
//                nullptr,
//                yPlane,
//                pCodecCtx->width,
//                uPlane,
//                uvPitch,
//                vPlane,
//                uvPitch
//        );
//        SDL_RenderClear(renderer);
//        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
//        SDL_RenderPresent(renderer);
//        printf("Current PTS = %d\n", CurrentPTS);
//    }
//
//    int ReadFrame() override
//    {
//        AVFrame *frame;
//        mu.lock();
//        if (vframe_buf.empty() && !flag)
//        {
//            mu.unlock();
//            Callback();
////            Vfinish = true;
////            status = FINISHED;
//
//            return 2;
//        }
//        if (vframe_buf.empty())
//        {
//            mu.unlock();
//            return 1;
//        }
//        frame = vframe_buf.front();
//        vframe_buf.pop_front();
//
//        CurrentPTS = frame->pts;
//        mu.unlock();
//        AVPicture pict;
//        pict.data[0] = yPlane;
//        pict.data[1] = uPlane;
//        pict.data[2] = vPlane;
//        pict.linesize[0] = pCodecCtx->width;
//        pict.linesize[1] = uvPitch;
//        pict.linesize[2] = uvPitch;
////                // Convert the image into YUV format that SDL uses
////        read_current_frame(frame, pict);
//        sws_scale(sws_ctx, (uint8_t const *const *) frame->data, frame->linesize, 0, pCodecCtx->height, pict.data,
//                  pict.linesize);
//        SDL_UpdateYUVTexture(
//                texture,
//                nullptr,
//                yPlane,
//                pCodecCtx->width,
//                uPlane,
//                uvPitch,
//                vPlane,
//                uvPitch
//        );
//
//        SDL_RenderClear(renderer);
//        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
//        SDL_RenderPresent(renderer);
////        printf("Current PTS = %d\n", CurrentPTS);
//        av_frame_unref(frame);
//        return 0;
//    }
//    void init_video(const char *url)
//    {
//        status = NOT_INITIALIZED;
//        if(avformat_open_input(&pFormatCtx, url, nullptr, nullptr)!=0)//argv[1]
//        {
//            printf("Couldn't open file\n");
//            return; // Couldn't open file
//        }
//
//        if(avformat_find_stream_info(pFormatCtx, nullptr)<0)
//        {
//            printf("Couldn't find stream information\n");
//            return; // Couldn't find stream information
//        }
//
//        av_dump_format(pFormatCtx, 0, url, 0);
//    }
//    void init_contexts()
//    {
//        videoStream=-1;
//        for(i=0; i<pFormatCtx->nb_streams; i++)
//            if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
//            {
//                videoStream=i;
//                break;
//            }
//        if(videoStream==-1)
//        {
//            printf("this is not a video");
//            return;
//        }
//
//
//        pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
//        pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
//        if(pCodec==nullptr)
//        {
//            fprintf(stderr, "Unsupported codec!\n");
//            return; // Codec not found
//        }
//        pCodecCtx = avcodec_alloc_context3(pCodec);
//        pParam = avcodec_parameters_alloc();
//        avcodec_parameters_from_context(pParam, pCodecCtxOrig);
//        avcodec_parameters_to_context(pCodecCtx, pParam);
//        avcodec_parameters_free(&pParam);
//        if(avcodec_open2(pCodecCtx, pCodec, nullptr)<0)
//            return;
//
//        sws_ctx = sws_getContext(pCodecCtx->width,
//                                 pCodecCtx->height,
//                                 pCodecCtx->pix_fmt,
//                                 pCodecCtx->width,
//                                 pCodecCtx->height,
//                                 AV_PIX_FMT_YUV420P,
//                                 SWS_BILINEAR,
//                                 nullptr,
//                                 nullptr,
//                                 nullptr
//        );
//    }
//    void init_SDL()
//    {
//        if(videoStream==-1)
//            return;
//        flag = true;
//        window = SDL_CreateWindow(
//                "SDL2Test",
//                SDL_WINDOWPOS_UNDEFINED,
//                SDL_WINDOWPOS_UNDEFINED,
//                pCodecCtx->width,
//                pCodecCtx->height,
//                0
//        );
//
//        renderer = SDL_CreateRenderer(window, -1, 0);
//        if (!renderer) {
//            fprintf(stderr, "SDL: could not create renderer - exiting\n");
//            exit(1);
//        }
//        texture = SDL_CreateTexture(
//                renderer,
//                SDL_PIXELFORMAT_YV12,
//                SDL_TEXTUREACCESS_STREAMING,
//                pCodecCtx->width,
//                pCodecCtx->height
//        );
//        if (!texture) {
//            fprintf(stderr, "SDL: could not create texture - exiting\n");
//            exit(1);
//        }
//        // set up YV12 pixel array (12 bits per pixel)
//        yPlaneSz = pCodecCtx->width * pCodecCtx->height;
//        uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
//        yPlane = (Uint8*)malloc(yPlaneSz);
//        uPlane = (Uint8*)malloc(uvPlaneSz);
//        vPlane = (Uint8*)malloc(uvPlaneSz);
//        if (!yPlane || !uPlane || !vPlane) {
//            fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
//            exit(1);
//        }
//        uvPitch = pCodecCtx->width / 2;
//    }
//    void write_frames() override
//    {
//        status = STARTED;
//        packet = av_packet_alloc();
//        while(av_read_frame(pFormatCtx, packet)>=0)
//        {
//            // Is this a packet from the video stream?
//            if(packet->stream_index==videoStream)
//            {
//                write_current_frame();
//            }
//        }
//        av_packet_unref(packet);
//        flag = false;
//    }
//    attribute_deprecated void read_frames()
//    {
//        //int PTS;
//        long double msec;
//        struct timespec time1, time2;
////        clock_gettime(CLOCK_REALTIME, &time2);
//        while (true)
//        {
//            while (VideoStop)
//            {
//                std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            }
//            AVFrame *frame;
//            mu.lock();
//            if (!flag && vframe_buf.empty())
//            {
//                SDL_DestroyTexture(texture);
//                SDL_DestroyRenderer(renderer);
//                SDL_DestroyWindow(window);
////                SDL_Quit();
//                mu.unlock();
//                Vfinish = true;
//                return;
//            }
//            if(vframe_buf.empty())
//            {
//                mu.unlock();
//                std::this_thread::sleep_for(std::chrono::milliseconds(2));
//                continue;
//            }
//            frame = vframe_buf.front();
//            vframe_buf.pop_front();
//            mu.unlock();
//
//            CurrentPTS = frame->pts;
////            while (true) {
////                clock_gettime(CLOCK_REALTIME, &time1);
////                msec = 1000 * (time1.tv_sec - time2.tv_sec) + (time1.tv_nsec - time2.tv_nsec) / 1000000;
////                if (CurrentPTS <= msec)
////                    break;
////            }
//            printf("Time from start video = %Lf\n", msec);
//            AVPicture pict;
//            pict.data[0] = yPlane;
//            pict.data[1] = uPlane;
//            pict.data[2] = vPlane;
//            pict.linesize[0] = pCodecCtx->width;
//            pict.linesize[1] = uvPitch;
//            pict.linesize[2] = uvPitch;
////                // Convert the image into YUV format that SDL uses
//            read_current_frame(frame, pict);
//            av_frame_unref(frame);
//            SDL_PollEvent(&event);
//            switch (event.type)
//            {
//                case SDL_QUIT:
//
//                    SDL_DestroyTexture(texture);
//                    SDL_DestroyRenderer(renderer);
//                    SDL_DestroyWindow(window);
//                    SDL_Quit();
//                    Vfinish = true;
//                    return;
//                default:
//                    break;
//            }
//        }
////        Vfinish = true;
//    }
//public:
//    int GetCurrentPTS() override
//    {
//        return CurrentPTS;
//    }
//    bool MediaFinished() override
//    {
//        return Vfinish;
//    }
//    MediaStatus GetStatus() override
//    {
//        return status;
//    }
//    void SetStatus(MediaStatus status) override
//    {
//        this->status = status;
//    }
//    CFFmpegVideo() = default;
//    void init(const char *url) override
//    {
//        init_video(url);
//        init_contexts();
//        init_SDL();
//    }
//    attribute_deprecated void play() override
//    {
//        if(videoStream==-1)
//            return;
//        thread1 = std::thread(&CFFmpegVideo::write_frames, this);
//        thread2 = std::thread(&CFFmpegVideo::read_frames, this);
//
//    }
//    void stop() override
//    {
////        thread1.join();
////        thread2.join();
//        free(yPlane);
//        free(uPlane);
//        free(vPlane);
//        avcodec_close(pCodecCtxOrig);
//        avcodec_close(pCodecCtx);
//        avformat_close_input(&pFormatCtx);
//    }
//};
//
//class KeyHandler
//{
//    int filefd = -1;
//    struct input_event event{};
//
//    char data[sizeof(event)]{};
//
//
//
//    std::atomic_bool session{};
//    bool MyKey = false;
//    std::function<void()> Callback;
//    std::thread thread;
//public:
//
//    void SetCallback(std::function<void()> fn)
//    {
//        Callback = std::move(fn);
//    }
//
//    void Init()
//    {
//        session = true;
//        thread = std::thread(&KeyHandler::Check, this);
//    }
//
//    void Stop()
//    {
//        session = false;
//        thread.join();
//    }
//    void Check()
//    {
////        file = std::ifstream("/dev/input/event5");
//        filefd = open("/dev/input/event5", O_NONBLOCK);
//
//        //check if file opened!
//        if(filefd == -1) {
//            printf("Unable to open file!\n");
//            return;
//        }
//
//        while(session) {
//
//            if (read(filefd, data, sizeof(event)) == -1 && errno == EAGAIN)
//            {
//
//                continue;
//            } else if (read(filefd, data, sizeof(event)) == -1 && errno != EAGAIN)
//            {
//                printf("Error reading file\n");
//                break;
//            }
////            file.read(data, sizeof(event))
//
//
//
//            memcpy(&event, data, sizeof(event));
//            if(event.type == EV_KEY) {
//
//                if(event.code == KEY_SPACE && event.value == 1)
//                {
//                    if (MyKey)
//                    {
////                    cout << "The Escape Key Was Pushed!" << endl;
////                    session = false;
//                        Callback();
////                        CMediaPlayer::Pause = false;
//                        MyKey = false;
//                    } else
//                    {
//                        Callback();
////                        CMediaPlayer::Pause = true;
//                        MyKey = true;
//                    }
//                }
//                else {
////                    cout << "Key Press " << event.code << endl;
//                }
//            }
//        }
//        close(filefd);
//    }
//};
//
//class CMediaPlayer
//{
//    IMedia *video{};
//    IMedia *audio{};
////    KeyHandler *keyHandler{};
//    std::thread th1;
//    std::thread th2;
//    bool Pause{};
//    void call_back_video()
//    {
//        printf("I am video callback!\n");
//        video->SetStatus(IMedia::FINISHED);
//    }
//    void call_back_audio()
//    {
//        printf("I am audio callback!\n");
//        audio->SetStatus(IMedia::FINISHED);
//
//    }
//public:
//    void call_back_pause()
//    {
//        if (!Pause)
//            Pause = true;
//        else
//            Pause = false;
//    }
//
//
//    CMediaPlayer() = default;
//    explicit CMediaPlayer(char *url)
//    {
//        video = new CFFmpegVideo;
//        audio = new CFFmpeg_Audio;
////        keyHandler = new KeyHandler;
//        video->init(url);
//        audio->init(url);
//        video->SetCallback(std::bind(&CMediaPlayer::call_back_video, this));
//        audio->SetCallback(std::bind(&CMediaPlayer::call_back_audio, this));
////        keyHandler->SetCallback(std::bind(&CMediaPlayer::call_back_pause, this));
//    }
//    ~CMediaPlayer() = default;
//
//
//
//    void Play()
//    {
//
//        th1 = std::thread(&IMedia::write_frames, video);
//        th2 = std::thread(&IMedia::write_frames, audio);
////        video->write_frames();
////        audio->write_frames();
//
//
//
//
//
//
//
//        SDL_PauseAudio(0);
//        while (video->GetStatus() != IMedia::FINISHED || audio->GetStatus() != IMedia::FINISHED)///!video->MediaFinished() && !audio->MediaFinished()
//        {
//
////            Pause = true;
//            while(Pause)
//            {
//                std::this_thread::sleep_for(std::chrono::milliseconds(2));
////                Pause = false;
//            }
//
//
//            if (audio->GetStatus() != IMedia::FINISHED)
//                audio->ReadFrame();
//            if (video->GetStatus() != IMedia::FINISHED)
//                if (video->GetCurrentPTS() <= audio->GetCurrentPTS())
//                    video->ReadFrame();
//
//
//
//
////            if (!audio->MediaFinished())
////                audio->ReadFrame();
////            if (!video->MediaFinished())
////                if (video->GetCurrentPTS() <= audio->GetCurrentPTS())
////                    video->ReadFrame();
//
//
//
//
////            if (audio->ReadFrame() < 2)
////            {
////                if (video->GetCurrentPTS() <= audio->GetCurrentPTS())
////                {
////                    if (video->ReadFrame() == 2)
////                    {
////                        break;
////                    }
////                }
////            }
//
//
//        }
////        SDL_DestroyTexture(texture);
////        SDL_DestroyRenderer(renderer);
////        SDL_DestroyWindow(window);
//        SDL_CloseAudio();//Close SDL
//        SDL_Quit();
//    }
//
//    void Stop()
//    {
//        th1.join();
//        th2.join();
//
//        video->stop();
//        audio->stop();
//    }
//};


//#include "CFFmpegAudio.h"
//#include "CFFmpegVideo.h"
#include "CMediaPlayer.h"
#include "KeyHandler.h"
#include "CConfig.h"


int main(int argc, char *argv[])
{

    CConfig::getInstance().initConfig();

////    char *url = CConfig::getInstance().getUrl();
    std::string url = CConfig::getInstance().getUrl();
    std::string Hotkey = CConfig::getInstance().getHotkey();
    auto *media = new CMediaPlayer(url);
    auto *k1 = new KeyHandler(Hotkey);
    k1->SetCallback(std::bind(&CMediaPlayer::call_back_pause, media));
////    k1->SetCallbackAL(std::bind(&IMedia::callbackL, media->getAudio()));
////    k1->SetCallbackAR(std::bind(&IMedia::callbackR, media->getAudio()));
    k1->SetCallbackVL(std::bind(&IMedia::callbackL, media->getVideo()));
    k1->SetCallbackVR(std::bind(&IMedia::callbackR, media->getVideo()));
    k1->Init();
////    k1->Check();
    media->Play();
    media->Stop();
    k1->Stop();
//
    delete media, k1;



//    IMedia *video = new CFFmpegVideo;
//    k1->SetCallbackVL(std::bind(&IMedia::callbackL, video));
//    k1->SetCallbackVR(std::bind(&IMedia::callbackR, video));
//    video->init("videoplayback");
//    video->play();
//    video->stop();

//auto *audio = new CFFmpegAudio;
//audio->init("music.mp3");
//audio->play();
//audio->stop();





    return 0;
}

























