//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_CFFMPEGVIDEO_H
#define VIDEO_TEST2_CFFMPEGVIDEO_H
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

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
static Uint8  *audio_chunk;
static Uint32  audio_len;
static Uint8  *audio_pos;



class CFFmpegVideo : public IMedia
{
    AVFormatContext *pFormatCtx{};
    int i{},videoStream{};
    AVCodecContext *pCodecCtxOrig{};
    AVCodecContext *pCodecCtx{};
    AVCodecContext *aCodecCtxOrig{};
    AVCodecContext *aCodecCtx{};
    AVCodec *pCodec{};
    AVCodec *aCodec{};
    AVCodecParameters *pParam{};
    AVCodecParameters *aParam{};
    AVFrame *pFrame{};
    AVFrame *aFrame{};
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
    std::mutex mu{}, mu_fmt_ctx{};
    std::deque<AVFrame*> vframe_buf{};
    bool flag{}, Vfinish{};
    std::thread thread1{};
    std::thread thread2{};
    long CurrentPTS{};
    long AudioPTS{};
    struct timespec time1{}, time2{};
    int t{};
    long double msec{};

    bool ctx_flag = false;







    int				 audioStream{};
    uint8_t			*out_buffer{};
    SDL_AudioSpec wanted_spec{};
    int ret{};
//    uint32_t len;
    int got_picture{};
    int index{};
    int64_t in_channel_layout{};
    struct SwrContext *au_convert_ctx{};
    int out_buffer_size{};

    std::deque<AVFrame*> aframe_buf{};
    bool Afinish{};









    static void fill_audio(void *udata,Uint8 *stream,int len);
    void write_current_frame();
    void write_audio_frame();

    attribute_deprecated void read_current_frame(AVFrame *frame, AVPicture pict);

    void callbackL() override;
    void callbackR() override;

    void init_video(std::string url);

    void init_contexts();

    void init_SDL();

    void write_frames() override;
    attribute_deprecated void read_frames();

public:

    long GetCurrentPTS() override;
    long GetAudioPTS() override;

    bool MediaFinished() override;

    MediaStatus GetStatus() override;

    void SetStatus(MediaStatus status) override;

    CFFmpegVideo() = default;
    void init(std::string url) override;

    attribute_deprecated void play() override;

    void stop() override;

    int ReadFrame() override;

    void SkipFrame() override;
    void SkipAudioFrame() override;

    void ReadAudioFrame() override;
};


#endif //VIDEO_TEST2_CFFMPEGVIDEO_H
