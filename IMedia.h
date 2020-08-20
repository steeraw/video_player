//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_IMEDIA_H
#define VIDEO_TEST2_IMEDIA_H


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

protected:

    MediaStatus status{};
public:
//    getStatus();
    virtual void init(std::basic_string<char> url) = 0;
    attribute_deprecated virtual void play() = 0;
    virtual void stop() = 0;
    virtual int GetCurrentPTS() = 0;
    virtual bool MediaFinished() = 0;
    virtual int ReadFrame() = 0;
    virtual void write_frames() = 0;
    virtual ~IMedia()= default;
    virtual MediaStatus GetStatus() = 0;
    virtual void SetStatus(MediaStatus mediaStatus) = 0;
};


#endif //VIDEO_TEST2_IMEDIA_H
