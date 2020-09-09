//
// Created by root on 19.08.20.
//

#include <iostream>
#include "CFFmpegVideo.h"


void CFFmpegVideo::fill_audio(void *udata, Uint8 *stream, int len)
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


void CFFmpegVideo::write_current_frame()
{
    pFrame=av_frame_alloc();
    avcodec_send_packet(pCodecCtx, packet);
    avcodec_receive_frame(pCodecCtx, pFrame);
///    pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[videoStream]->time_base, {1,1000});
//    pFrame->pts = packet->pts;
//    av_seek_frame(pFormatCtx, videoStream, pFrame->coded_picture_number, AVSEEK_FLAG_FRAME);
    while (vframe_buf.size() > 200)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    mu.lock();
    vframe_buf.push_back(pFrame);
    mu.unlock();
}

attribute_deprecated void CFFmpegVideo::read_current_frame(AVFrame *frame, AVPicture pict)
{
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
    printf("Current PTS = %d\n", CurrentPTS);
}
int CFFmpegVideo::ReadFrame()
{
    AVFrame *frame;
    mu.lock();
    if (vframe_buf.empty() && !flag)
    {
        mu.unlock();
        Callback();
//            Vfinish = true;
//            status = FINISHED;

        return 2;
    }
    if (vframe_buf.empty())
    {
        mu.unlock();
        return 1;
    }
    frame = vframe_buf.front();
    vframe_buf.pop_front();

    CurrentPTS = av_rescale_q(frame->pts, pFormatCtx->streams[videoStream]->time_base, {1,100000});
    mu.unlock();
    AVPicture pict;
    pict.data[0] = yPlane;
    pict.data[1] = uPlane;
    pict.data[2] = vPlane;
    pict.linesize[0] = pCodecCtx->width;
    pict.linesize[1] = uvPitch;
    pict.linesize[2] = uvPitch;
//                // Convert the image into YUV format that SDL uses
//        read_current_frame(frame, pict);
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
//        printf("Current PTS = %d\n", CurrentPTS);
    av_frame_unref(frame);
    return 0;
}
void CFFmpegVideo::init_video(std::string url)
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
void CFFmpegVideo::init_contexts()
{
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoStream=i;
            break;
        }
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

    if(videoStream==-1)
    {
        no_media = true;
        printf("this is not a video");
        return;
    }



    aCodecCtxOrig = pFormatCtx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtxOrig->codec_id);
    if (aCodec == nullptr) {
        fprintf(stderr, "Unsupported codec!\n");
        return; // Codec not found
    }
    aCodecCtx = avcodec_alloc_context3(aCodec);
    aParam = avcodec_parameters_alloc();
    avcodec_parameters_from_context(aParam, aCodecCtxOrig);
    avcodec_parameters_to_context(aCodecCtx, aParam);
    avcodec_parameters_free(&aParam);
    if (avcodec_open2(aCodecCtx, aCodec, nullptr) < 0)
        return;

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));////////////////////////OPTIONAL?
    av_init_packet(packet);////////////////////////////////////////////////OPTIONAL?

//    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);
    out_buffer_size = av_samples_get_buffer_size(nullptr, 2, aCodecCtx->frame_size, AV_SAMPLE_FMT_S16, 1);//AV_SAMPLE_FMT_S16


    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
//    pFrame=av_frame_alloc();

    //FIX:Some Codec's Context Information is missing
    in_channel_layout=av_get_default_channel_layout(aCodecCtx->channels);








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
void CFFmpegVideo::init_SDL()
{
    if(videoStream==-1)
    {

        return;
    }

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










    if(SDL_Init(SDL_INIT_AUDIO))
    {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return;
    }
    //SDL_AudioSpec
    wanted_spec.freq = aCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;//AUDIO_S16SYS
    wanted_spec.channels = aCodecCtx->channels;
//    wanted_spec.silence = 0;
    wanted_spec.samples = aCodecCtx->frame_size;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = aCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, nullptr)<0){
        printf("can't open audio.\n");
        exit(1);
    }
    au_convert_ctx = swr_alloc();
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, aCodecCtx->sample_rate,
                                      in_channel_layout,aCodecCtx->sample_fmt , aCodecCtx->sample_rate,0, nullptr);
    swr_init(au_convert_ctx);
}
void CFFmpegVideo::write_frames()
{
    status = STARTED;
    packet = av_packet_alloc();
//    int c = 0;

    int x = 0;
    while (x >= 0)
    {
        mu_fmt_ctx.lock();
        x = av_read_frame(pFormatCtx, packet);
        if (x >= 0 && !ctx_flag)
        {
            if(packet->stream_index==videoStream)
            {
                write_current_frame();
            }
            if (packet->stream_index == audioStream)
            {
                write_audio_frame();
            }
            av_packet_unref(packet);
        }
        mu_fmt_ctx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
//        usleep(10);
    }





////    mu_fmt_ctx.lock();
//    while(av_read_frame(pFormatCtx, packet)>=0)
//    {
//
////        mu_fmt_ctx.unlock();
//        // Is this a packet from the video stream?
//        if(packet->stream_index==videoStream)
//        {
//            write_current_frame();
////            c++;
//        }
//
//        if (packet->stream_index == audioStream)
//        {
//            write_audio_frame();
//
//        }
//
//        av_packet_unref(packet);
////        mu_fmt_ctx.lock();
//    }
////    mu_fmt_ctx.unlock();
//    av_packet_unref(packet);
//    printf("video: %d\n",t);




//    t = 0;
//    time1 = (float)pFormatCtx->duration / 1000000;
//    fps = pFormatCtx->streams[audioStream]->nb_frames / time1;
//
//    ts = (float)pCodecCtx->time_base.den / fps;
//    float sec = 0.0;
//    while (av_read_frame(pFormatCtx, packet) >= 0)
//    {
//        if (packet->stream_index == audioStream)
//        {
//            write_audio_frame();
//
//        }
//        av_packet_unref(packet);
//    }
//    printf("audio: %d\n",t);
    flag = false;



}
attribute_deprecated void CFFmpegVideo::read_frames()
{
    //int PTS;


    clock_gettime(CLOCK_REALTIME, &time2);
    while (true)
    {
//            while (VideoStop)
//            {
//                std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            }
        AVFrame *frame;
        mu.lock();
        if (!flag && vframe_buf.empty())
        {
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
                SDL_Quit();
            mu.unlock();
            Vfinish = true;
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

        CurrentPTS = av_rescale_q(frame->pts, pFormatCtx->streams[videoStream]->time_base, {1,1000});
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                clock_gettime(CLOCK_REALTIME, &time1);
                msec = 1000 * (time1.tv_sec - time2.tv_sec) + (time1.tv_nsec - time2.tv_nsec) / 1000000;
                msec += t;
                if (CurrentPTS > msec + 500)
                {
                    av_frame_unref(frame);
                    goto Continue;
                }
                if (CurrentPTS <= msec)
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
        read_current_frame(frame, pict);
        av_frame_unref(frame);
        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:

                SDL_DestroyTexture(texture);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                Vfinish = true;
                return;
            default:
                break;
        }
        Continue:
        continue;
    }
//        Vfinish = true;
}
long CFFmpegVideo::GetCurrentPTS()
{
    return CurrentPTS;
}
long CFFmpegVideo::GetAudioPTS() {
    return AudioPTS;
}

bool CFFmpegVideo::MediaFinished()
{
    return Vfinish;
}
IMedia::MediaStatus CFFmpegVideo::GetStatus()
{
    return status;
}
void CFFmpegVideo::SetStatus(MediaStatus status)
{
    this->status = status;
}
void CFFmpegVideo::init(std::string url)
{
    init_video(url);
    init_contexts();
    init_SDL();
}
attribute_deprecated void CFFmpegVideo::play()
{
    thread1 = std::thread(&CFFmpegVideo::write_frames, this);
    thread2 = std::thread(&CFFmpegVideo::read_frames, this);
    thread1.join();
    thread2.join();
}
void CFFmpegVideo::stop()
{
    free(yPlane);
    free(uPlane);
    free(vPlane);
    avcodec_close(pCodecCtxOrig);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

int CFFmpegVideo::SkipFrame()
{
//    if (vframe_buf.size() > 1)
//    {
    if (!vframe_buf.empty())
    {
        AVFrame *frame;
        mu.lock();
        frame = vframe_buf.front();
        CurrentPTS = av_rescale_q(frame->pts, pFormatCtx->streams[videoStream]->time_base, {1,100000});
        av_frame_unref(vframe_buf.front());
        vframe_buf.pop_front();



        mu.unlock();
        return 0;
    }
    return 1;

//    }
}

int CFFmpegVideo::SkipAudioFrame()
{
    if (!aframe_buf.empty())
    {
        AVFrame *frame;
        mu.lock();
        frame = aframe_buf.front();
        AudioPTS = av_rescale_q(frame->pts, pFormatCtx->streams[audioStream]->time_base, {1,100000});
        av_frame_unref(vframe_buf.front());
        aframe_buf.pop_front();
        mu.unlock();
        return 0;
    }
    return 1;
}


void CFFmpegVideo::callbackL() {
    mu_fmt_ctx.lock();
    mu.lock();
    struct timespec time_bb, time_aa;
    clock_gettime(CLOCK_REALTIME, &time_bb);
    for (auto it : vframe_buf)
    {
        av_frame_unref(it);
    }
    for (auto it : aframe_buf)
    {
        av_frame_unref(it);
    }
    vframe_buf.clear();
    aframe_buf.clear();
//    int vrew = 1000000;
    CurrentPTS -= REWIND;
    AudioPTS -= REWIND;
//    t -= rew;



    ctx_flag = true;



//    avformat_seek_file(pFormatCtx,
//                       audioStream,
//                       0,
//                       av_rescale_q(AudioPTS, {1,100000},pFormatCtx->streams[audioStream]->time_base),
//                       av_rescale_q(AudioPTS, {1,100000},pFormatCtx->streams[audioStream]->time_base),
//                       0);
    av_seek_frame(pFormatCtx,
                  videoStream,
                  av_rescale_q(CurrentPTS, {1,100000},
                               pFormatCtx->streams[videoStream]->time_base),
                               0);
//    av_seek_frame(pFormatCtx, audioStream, av_rescale_q(AudioPTS, {1,100000},pFormatCtx->streams[audioStream]->time_base),   AVSEEK_FLAG_BACKWARD);
//    avcodec_flush_buffers(pCodecCtx);
//    avcodec_flush_buffers(aCodecCtx);

    clock_gettime(CLOCK_REALTIME, &time_aa);


//    printf("Time to seek back: %ld", 1000 * (time_a.tv_sec - time_b.tv_sec) + (time_a.tv_nsec - time_b.tv_nsec) / 1000000);
    mu.unlock();
    mu_fmt_ctx.unlock();
    ctx_flag = false;
    long time = 100000 * (time_aa.tv_sec - time_bb.tv_sec) + ((time_aa.tv_nsec - time_bb.tv_nsec) / 10000);
//    printf("Time to seek back: %ld", time);
    std::cout << "Time to seek back:" << time << std:: endl;
}

void CFFmpegVideo::callbackR() {
    mu_fmt_ctx.lock();
    mu.lock();
    struct timespec time_b, time_a;
    clock_gettime(CLOCK_REALTIME, &time_b);
    for (auto it : vframe_buf)
    {
        av_frame_unref(it);
    }
    for (auto it : aframe_buf)
    {
        av_frame_unref(it);
    }
    vframe_buf.clear();
    aframe_buf.clear();

//    int vrew = 1000000;
    CurrentPTS += REWIND;
    AudioPTS += REWIND;
//    t -= rew;


//    ctx_flag = true;
//    avformat_seek_file(pFormatCtx,
//                       audioStream,
//                       0,
//                       av_rescale_q(AudioPTS, {1,100000},pFormatCtx->streams[audioStream]->time_base),
//                       av_rescale_q(AudioPTS, {1,100000},pFormatCtx->streams[audioStream]->time_base),
//                       0);



    av_seek_frame(pFormatCtx,
                  videoStream,
                  av_rescale_q(CurrentPTS, {1,100000},
                               pFormatCtx->streams[videoStream]->time_base),
                               AVSEEK_FLAG_BACKWARD);
//    av_seek_frame(pFormatCtx, audioStream, av_rescale_q(AudioPTS, {1,100000},pFormatCtx->streams[audioStream]->time_base),  AVSEEK_FLAG_BACKWARD);
//    avcodec_flush_buffers(pCodecCtx);
//    avcodec_flush_buffers(aCodecCtx);

//    ctx_flag = false;

    clock_gettime(CLOCK_REALTIME, &time_a);
    mu.unlock();
    mu_fmt_ctx.unlock();
    long time = 100000 * (time_a.tv_sec - time_b.tv_sec) + ((time_a.tv_nsec - time_b.tv_nsec) / 10000);

    std::cout << "Time to seek front:" << time << std:: endl;
//    printf("Time to seek front: %ld", time);
    //    av_seek_frame(pFormatCtx, videoStream, av_rescale_q(CurrentPTS, {1,100000},pFormatCtx->streams[videoStream]->time_base), AVSEEK_FLAG_BACKWARD);

}

void CFFmpegVideo::write_audio_frame() {

    aFrame=av_frame_alloc();
    avcodec_send_packet(aCodecCtx, packet);

    avcodec_receive_frame(aCodecCtx, aFrame);
//        pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, {1,1000});
//        Pts = pFrame->pts;
//        av_seek_frame(pFormatCtx, audioStream, 0, AVSEEK_FLAG_FRAME);
//        pFrame->coded_picture_number
    while (aframe_buf.size() > 500)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    mu.lock();
    aframe_buf.push_back(aFrame);
    mu.unlock();

}

void CFFmpegVideo::ReadAudioFrame() {

        AVFrame *frame;
        mu.lock();
        if (aframe_buf.empty() && !flag)
        {
            mu.unlock();
//            Afinish = true;
            Callback();
//            status = FINISHED;
            return;
        }
        if (aframe_buf.empty())
        {
            mu.unlock();
            return;
        }

        frame = aframe_buf.front();
        aframe_buf.pop_front();
        mu.unlock();
        AudioPTS = av_rescale_q(frame->pts, pFormatCtx->streams[audioStream]->time_base, {1,100000});
        swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);

        while(audio_len>0)//Wait until finish
            SDL_Delay(1);

        audio_chunk = (Uint8 *) out_buffer;
        audio_len = out_buffer_size;
        audio_pos = audio_chunk;
        av_frame_unref(frame);
//        Callback();



}




