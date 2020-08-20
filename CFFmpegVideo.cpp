//
// Created by root on 19.08.20.
//

#include "CFFmpegVideo.h"

void CFFmpegVideo::write_current_frame()
{
    pFrame=av_frame_alloc();
    avcodec_send_packet(pCodecCtx, packet);
    avcodec_receive_frame(pCodecCtx, pFrame);
    pFrame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[videoStream]->time_base, {1,1000});

    while (vframe_buf.size() > 100)
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

    CurrentPTS = frame->pts;
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
void CFFmpegVideo::init_SDL()
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
void CFFmpegVideo::write_frames()
{
status = STARTED;
packet = av_packet_alloc();
while(av_read_frame(pFormatCtx, packet)>=0)
{
// Is this a packet from the video stream?
if(packet->stream_index==videoStream)
{
write_current_frame();
}
}
av_packet_unref(packet);
flag = false;
}
attribute_deprecated void CFFmpegVideo::read_frames()
{
    //int PTS;
    long double msec;
    struct timespec time1, time2;
//        clock_gettime(CLOCK_REALTIME, &time2);
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
//                SDL_Quit();
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

        CurrentPTS = frame->pts;
//            while (true) {
//                clock_gettime(CLOCK_REALTIME, &time1);
//                msec = 1000 * (time1.tv_sec - time2.tv_sec) + (time1.tv_nsec - time2.tv_nsec) / 1000000;
//                if (CurrentPTS <= msec)
//                    break;
//            }
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
    }
//        Vfinish = true;
}
int CFFmpegVideo::GetCurrentPTS()
{
    return CurrentPTS;
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
    if(videoStream==-1)
        return;
    thread1 = std::thread(&CFFmpegVideo::write_frames, this);
    thread2 = std::thread(&CFFmpegVideo::read_frames, this);

}
void CFFmpegVideo::stop()
{
//        thread1.join();
//        thread2.join();
    free(yPlane);
    free(uPlane);
    free(vPlane);
    avcodec_close(pCodecCtxOrig);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}