//
// Created by root on 19.08.20.
//

#include "KeyHandler.h"

void KeyHandler::SetCallback(std::function<void()> fn)
{
    Callback = std::move(fn);
}
void KeyHandler::SetCallbackAL(std::function<void()> fn)
{
    CallbackAL = std::move(fn);
}
void KeyHandler::SetCallbackAR(std::function<void()> fn)
{
    CallbackAR = std::move(fn);
}
void KeyHandler::SetCallbackVL(std::function<void()> fn)
{
    CallbackVL = std::move(fn);
}
void KeyHandler::SetCallbackVR(std::function<void()> fn)
{
    CallbackVR = std::move(fn);
}
void KeyHandler::Init()
{
    if (key_map.find(MyHotKey) == key_map.end())//(!key_map[doc["Hotkey"].GetString()])//
    {
        printf("Hotkey incorrect, check your config. You can select Enter, Tab, Space, Esc, Backspace\n");
        exit(1);
    }
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

    while(session)
    {

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

            if(event.code == key_map[MyHotKey] && event.value == 1)
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
            else if(event.code == KEY_TAB && event.value ==1)
            {
                ////REWIND

                CallbackVL();
                CallbackAL();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else if(event.code == KEY_ENTER && event.value ==1)
            {
                ////REWIND

                CallbackVR();
                CallbackAR();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
//            else
//            {
////                    cout << "Key Press " << event.code << endl;
//            }
        }
    }
    close(filefd);
}

KeyHandler::KeyHandler(std::string Key)
{
    MyHotKey = Key;
}




