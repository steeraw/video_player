//
// Created by root on 19.08.20.
//

#ifndef VIDEO_TEST2_KEYHANDLER_H
#define VIDEO_TEST2_KEYHANDLER_H

#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <atomic>
#include <linux/input.h>
#include <functional>
#include <thread>
#include <map>
class KeyHandler
{
    int filefd = -1;
    struct input_event event{};

    char data[sizeof(event)]{};

    std::atomic_bool session{};
    bool MyKey = false;
    std::function<void()> Callback;
    std::thread thread;
    std::string MyHotKey;
    std::map<std::string, int> key_map = {
            {"Tab", KEY_TAB},
            {"Enter", KEY_ENTER},
            {"Space", KEY_SPACE},
            {"Esc", KEY_ESC},
            {"Backspace", KEY_BACKSPACE},
            {"NONE", 0}
    };
public:
    explicit KeyHandler(std::string Key);
    void SetCallback(std::function<void()> fn);


    void Init();


    void Stop();

    void Check();

};


#endif //VIDEO_TEST2_KEYHANDLER_H
