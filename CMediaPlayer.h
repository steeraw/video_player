//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_CMEDIAPLAYER_H
#define VIDEO_TEST2_CMEDIAPLAYER_H


//#include <thread>
#include "IMedia.h"
#include "CFFmpegVideo.h"
#include "CFFmpegAudio.h"

class CMediaPlayer
{
    IMedia *video{};
    IMedia *audio{};
//    KeyHandler *keyHandler{};
    std::thread th1;
    std::thread th2;
    bool Pause{};
    void call_back_video();

    void call_back_audio();

public:
    void call_back_pause();



    CMediaPlayer() = default;
    explicit CMediaPlayer(char *url);

    ~CMediaPlayer() = default;



    void Play();


    void Stop();

};


#endif //VIDEO_TEST2_CMEDIAPLAYER_H
