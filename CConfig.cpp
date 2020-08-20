//
// Created by user on 20.08.20.
//

#include "CConfig.h"


std::string CConfig::getUrl() {
    return doc["inputStream"].GetString();
}

void CConfig::initConfig() {
    if (!ifs.is_open())
    {
        printf("Error opening file\n");
        return;
    }
    doc.ParseStream(isw);
//    if (key_map.find(doc["Hotkey"].GetString()) == key_map.end())//(!key_map[doc["Hotkey"].GetString()])//
//    {
//        printf("Hotkey incorrect, check your config. You can select Enter, Tab, Space, Esc, Backspace\n");
//        exit(1);
//    }
}

std::string CConfig::getHotkey() {
    return doc["Hotkey"].GetString();
}
