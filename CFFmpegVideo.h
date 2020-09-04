//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_CFFMPEGVIDEO_H
#define VIDEO_TEST2_CFFMPEGVIDEO_H

extern "C"
{
//#include "libavcodec/avcodec.h"//?
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
//#include "libavutil/imgutils.h"
#include <libswresample/swresample.h>
}
#include <SDL2/SDL.h>
#include <mutex>
#include <deque>
#include <thread>
#include "IMedia.h"

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
    std::mutex mu{};
    std::deque<AVFrame*> vframe_buf{};
    bool flag{}, Vfinish{};
    std::thread thread1{};
    std::thread thread2{};
    int CurrentPTS{};
    struct timespec time1, time2;
    int t;
    long double msec;
    void write_current_frame();

    attribute_deprecated void read_current_frame(AVFrame *frame, AVPicture pict);

    void SkipFrame() override;
    void callbackL() override;
    void callbackR() override;

    int ReadFrame() override;

    void init_video(std::string url);

    void init_contexts();

    void init_SDL();

    void write_frames() override;
    attribute_deprecated void read_frames();

public:

    int GetCurrentPTS() override;

    bool MediaFinished() override;

    MediaStatus GetStatus() override;

    void SetStatus(MediaStatus status) override;

    CFFmpegVideo() = default;
    void init(std::string url) override;

    attribute_deprecated void play() override;

    void stop() override;

};


#endif //VIDEO_TEST2_CFFMPEGVIDEO_H
