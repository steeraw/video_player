//
// Created by root on 19.08.20.
//

#include "KeyHandler.h"

void KeyHandler::SetCallback(std::function<void()> fn)
{
    Callback = std::move(fn);
}
void KeyHandler::Init()
{
    session = true;
    thread = std::thread(&KeyHandler::Check, this);
}
void KeyHandler::Stop()
{
    session = false;
    thread.join();
}
void KeyHandler::Check()
{
//        file = std::ifstream("/dev/input/event5");
    filefd = open("/dev/input/event5", O_NONBLOCK);

    //check if file opened!
    if(filefd == -1) {
        printf("Unable to open file!\n");
        return;
    }

    while(session) {

        if (read(filefd, data, sizeof(event)) == -1 && errno == EAGAIN)
        {

            continue;
        } else if (read(filefd, data, sizeof(event)) == -1 && errno != EAGAIN)
        {
            printf("Error reading file\n");
            break;
        }
//            file.read(data, sizeof(event))



        memcpy(&event, data, sizeof(event));
        if(event.type == EV_KEY) {

            if(event.code == KEY_SPACE && event.value == 1)
            {
                if (MyKey)
                {
//                    cout << "The Escape Key Was Pushed!" << endl;
//                    session = false;
                    Callback();
//                        CMediaPlayer::Pause = false;
                    MyKey = false;
                } else
                {
                    Callback();
//                        CMediaPlayer::Pause = true;
                    MyKey = true;
                }
            }
            else {
//                    cout << "Key Press " << event.code << endl;
            }
        }
    }
    close(filefd);
}


