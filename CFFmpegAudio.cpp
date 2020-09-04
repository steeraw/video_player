//
// Created by root on 19.08.20.
//

#include "CFFmpegAudio.h"

void CFFmpegAudio::fill_audio(void *udata, Uint8 *stream, int len)
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

void CFFmpegAudio::write_current_frame()
{
        pFrame=av_frame_alloc();
        avcodec_send_packet(pCodecCtx, packet);
        avcodec_receive_frame(pCodecCtx, pFrame);
        pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, {1,1000});
        Pts = pFrame->pts;
//        av_seek_frame(pFormatCtx, audioStream, 0, AVSEEK_FLAG_FRAME);
//        pFrame->coded_picture_number
        while (aframe_buf.size() > 100)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        mu.lock();
        aframe_buf.push_back(pFrame);
        mu.unlock();

}



void CFFmpegAudio::read_current_frame(AVFrame *frame)
{
    CurrentPTS = frame->pts;
    swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);

    while(audio_len>0)//Wait until finish
        SDL_Delay(1);

    audio_chunk = (Uint8 *) out_buffer;
    audio_len = out_buffer_size;
    audio_pos = audio_chunk;
}

int CFFmpegAudio::ReadFrame()
{
    AVFrame *frame;
    mu.lock();
    if (aframe_buf.empty() && !flag)
    {
        mu.unlock();
//            Afinish = true;
        Callback();
//            status = FINISHED;
        return 2;
    }
    if (aframe_buf.empty())
    {
        mu.unlock();
        return 1;
    }

    frame = aframe_buf.front();
    aframe_buf.pop_front();
    mu.unlock();
    CurrentPTS = frame->pts;
    swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);

    while(audio_len>0)//Wait until finish
        SDL_Delay(1);

    audio_chunk = (Uint8 *) out_buffer;
    audio_len = out_buffer_size;
    audio_pos = audio_chunk;
    av_frame_unref(frame);
//        Callback();
    return 0;
}

void CFFmpegAudio::SkipFrame()
{
    mu.lock();
    if (aframe_buf.empty())
    {
        mu.unlock();
        return;
    }
    aframe_buf.pop_front();
    mu.unlock();
}

void CFFmpegAudio::init_audio(std::string url)
{
    status = NOT_INITIALIZED;
    if(avformat_open_input(&pFormatCtx, url.c_str(), nullptr, nullptr)!=0)//argv[1]
    {
        printf("Couldn't open file\n");
        return; // Couldn't open file
    }

    if(avformat_find_stream_info(pFormatCtx, nullptr)<0)
    {
        printf("Couldn't find stream information\n");
        return; // Couldn't find stream information
    }

    av_dump_format(pFormatCtx, 0, url.c_str(), 0);
}

void CFFmpegAudio::init_contexts()
{
    audioStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            break;
        }
    if (audioStream == -1)
    {
        printf("No Audio\n");
        no_media = true;
        return;
    }


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

void CFFmpegAudio::init_SDL()
{
    if(audioStream==-1)
    {
        no_media = true;
        return;
    }

    flag = true;
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

void CFFmpegAudio::write_frames()
{
    t = 0;
    time1 = (float)pFormatCtx->duration / 1000000;
    fps = pFormatCtx->streams[audioStream]->nb_frames / time1;
//    ts = av_samples_get_buffer_size(nullptr, 2, 960 ,pFormatCtx->streams[audioStream]->codec->sample_fmt, 0);
    ts = (float)pCodecCtx->time_base.den / fps;
    tot = fps*ts*FNUM;


//    av_seek_frame(pFormatCtx, audioStream, tot, AVSEEK_FLAG_BACKWARD);
//    pFormatCtx->duration;
//    if (avformat_seek_file(pFormatCtx, audioStream, 0, FNUM*2048, FNUM*2049, AVSEEK_FLAG_FRAME))
//        printf("e-r-r-o-r\n");

//    t = (double)pCodecCtx->time_base.num / (double)pCodecCtx->time_base.den;
    float sec = 0.0;
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
//        sec = ((float)packet->pts / 1024) / fps;

        if (packet->stream_index == audioStream)
        {



            write_current_frame();
//            av_seek_frame(pFormatCtx, audioStream, packet->pts+ts, AVSEEK_FLAG_FRAME);
            t++;
        }
        av_packet_unref(packet);
    }
    printf("audio: %d\n",t);
    flag = false;
}

void CFFmpegAudio::read_frames()
{
    SDL_PauseAudio(0);

    while (true)
    {
        mu.lock();
        if (aframe_buf.empty() && !flag)
        {
            mu.unlock();
//                SDL_CloseAudio();//Close SDL
//                SDL_Quit();
            break;
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
        read_current_frame(frame);
        av_frame_unref(frame);
    }
    Afinish = true;
    SDL_CloseAudio();//Close SDL
    SDL_Quit();
}

int CFFmpegAudio::GetCurrentPTS()
{
    return CurrentPTS;
}

bool CFFmpegAudio::MediaFinished()
{
    return Afinish;
}

void CFFmpegAudio::init(std::string url)
{
    init_audio(url);
    init_contexts();
    init_SDL();
}

void CFFmpegAudio::play()
{
    thread1 = std::thread(&CFFmpegAudio::write_frames, this);
    thread2 = std::thread(&CFFmpegAudio::read_frames, this);
    thread1.join();
    thread2.join();
}

void CFFmpegAudio::stop()
{

    swr_free(&au_convert_ctx);
    av_free(out_buffer);
    avcodec_close(pCodecCtxOrig);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

IMedia::MediaStatus CFFmpegAudio::GetStatus()
{
    return status;
}

void CFFmpegAudio::SetStatus(IMedia::MediaStatus status)
{
    this->status = status;
}

void CFFmpegAudio::callbackL()
{
    CurrentPTS -= 1000;
    av_seek_frame(pFormatCtx, audioStream, ((double)(CurrentPTS)*fps*ts/1000), AVSEEK_FLAG_BACKWARD);
    mu.lock();
    aframe_buf.clear();
    mu.unlock();
}

void CFFmpegAudio::callbackR()
{
    mu.lock();
    aframe_buf.clear();
    mu.unlock();
    CurrentPTS += 1000;
    av_seek_frame(pFormatCtx, audioStream, ((double)(CurrentPTS)*fps*ts/1000), 0);
}




