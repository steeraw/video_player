//
// Created by root on 19.08.20.
//

#include "CMediaPlayer.h"

void CMediaPlayer::call_back_video()
{
    printf("I am video callback!\n");
    video->SetStatus(IMedia::FINISHED);
}
void CMediaPlayer::call_back_audio()
{
    printf("I am audio callback!\n");
    audio->SetStatus(IMedia::FINISHED);

}
void CMediaPlayer::call_back_pause()
{
    if (!Pause)
        Pause = true;
    else
        Pause = false;
}
CMediaPlayer::CMediaPlayer(std::string url)
{
    video = new CFFmpegVideo;
//    audio = new CFFmpegAudio;
//        keyHandler = new KeyHandler;
    video->init(url);
//    audio->init(url);
    video->SetCallback(std::bind(&CMediaPlayer::call_back_video, this));
//    audio->SetCallback(std::bind(&CMediaPlayer::call_back_audio, this));
//        keyHandler->SetCallback(std::bind(&CMediaPlayer::call_back_pause, this));
}
void CMediaPlayer::Play()
{
//    video->play();
//    int i = 0;
//    if (!have_video())
//    {
//        audio->play();
//    }
//    if (!have_audio())
//    {
//        video->play();
//    }
//    else
//    {
        th1 = std::thread(&IMedia::write_frames, video);
//        th2 = std::thread(&IMedia::write_frames, audio);
//        video->write_frames();
//        audio->write_frames();
        SDL_PauseAudio(0);
        while (video->GetStatus() != IMedia::FINISHED)///!video->MediaFinished() && !audio->MediaFinished()
        {
//            Pause = true;
            while(Pause)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
//                Pause = false;
            }



            video->ReadAudioFrame();

//            if (video->GetCurrentPTS() + DELAY_BIG < video->GetAudioPTS())
//            {
//                video->SkipAudioFrame();
//            }
            while (video->GetCurrentPTS() + DELAY_SMALL < video->GetAudioPTS())
            {
                video->SkipFrame();
            }
            while (video->GetCurrentPTS() > video->GetAudioPTS() + DELAY_SMALL)
            {
                video->SkipFrame();
            }


            if (video->GetCurrentPTS() <= video->GetAudioPTS())
            {
                video->ReadFrame();
            }
        }
//        SDL_DestroyTexture(texture);
//        SDL_DestroyRenderer(renderer);
//        SDL_DestroyWindow(window);
        SDL_CloseAudio();//Close SDL
        SDL_Quit();
//    }
}
void CMediaPlayer::Stop()
{
//    if (have_audio() && have_video())
//    {
//        th1.join();
//        th2.join();
//    }
    th1.join();
//
//    if (have_video())
//        video->stop();
//    if (have_audio())
//        audio->stop();
//    video->stop();
}

bool CMediaPlayer::have_video() {
    if (video->no_media)
        return false;
    return true;
}

bool CMediaPlayer::have_audio() {
    if (audio->no_media)
        return false;
    return true;
}

IMedia* CMediaPlayer::getAudio() {
    return audio;
}

IMedia *CMediaPlayer::getVideo() {
    return video;
}


