//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_IMEDIA_H
#define VIDEO_TEST2_IMEDIA_H
//10 seconds
#define REWIND 1000000
//0.3 seconds
#define DELAY_SMALL 30000
//7 seconds
#define DELAY_BIG 700000


#include <functional>
#include <libavutil/attributes.h>


class IMedia
{
public:

    enum MediaStatus
    {
        NOT_INITIALIZED,
        STARTED,
        FINISHED
    };

    std::function<void()> Callback{};
    void SetCallback(std::function<void()> fn)
    {
        Callback = std::move(fn);
    }

    bool no_media;
protected:
    MediaStatus status{};
public:
//    getStatus();
    virtual void init(std::basic_string<char> url) = 0;
    attribute_deprecated virtual void play() = 0;
    virtual void stop() = 0;
    virtual long GetCurrentPTS() = 0;
    virtual bool MediaFinished() = 0;
    virtual int SkipFrame() = 0;
    virtual int ReadFrame() = 0;
    virtual void write_frames() = 0;
    virtual ~IMedia()= default;
    virtual MediaStatus GetStatus() = 0;
    virtual void SetStatus(MediaStatus mediaStatus) = 0;
    virtual void callbackL() = 0;
    virtual void callbackR() = 0;
    virtual long GetAudioPTS() = 0;
    virtual int SkipAudioFrame() = 0;

    virtual void ReadAudioFrame() = 0;
};


#endif //VIDEO_TEST2_IMEDIA_H
