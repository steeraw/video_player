//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_CFFMPEGAUDIO_H
#define VIDEO_TEST2_CFFMPEGAUDIO_H


extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <libswresample/swresample.h>
}
#include <SDL_audio.h>
#include <SDL2/SDL.h>
#include <mutex>
#include <deque>
#include <thread>
#include "IMedia.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
static Uint8  *audio_chunk;
static Uint32  audio_len;
static Uint8  *audio_pos;




class CFFmpegAudio : public IMedia
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
    std::mutex mu{};
    std::deque<AVFrame*> aframe_buf{};
    bool flag{}, Afinish{};
    AVCodecContext *pCodecCtxOrig{};
    AVCodecParameters *pParam{};
    std::thread thread1{};
    std::thread thread2{};
    int CurrentPTS{};

    static void fill_audio(void *udata,Uint8 *stream,int len);

    void write_current_frame();

    attribute_deprecated void read_current_frame(AVFrame *frame);

    int ReadFrame() override;

    void SkipFrame() override;

    void init_audio(std::string url);

    void init_contexts();

    void init_SDL();

    void write_frames() override;

    attribute_deprecated void read_frames();

public:

    CFFmpegAudio() = default;
    int GetCurrentPTS() override;

    bool MediaFinished() override;

    void init(std::string url) override;

    attribute_deprecated void play() override;

    void stop() override;

    MediaStatus GetStatus() override;

    void SetStatus(MediaStatus status) override;

};


#endif //VIDEO_TEST2_CFFMPEGAUDIO_H
