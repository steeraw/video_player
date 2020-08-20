//
// Created by user on 20.08.20.
//

#ifndef SINGLETON_TEST_CCONFIG_H
#define SINGLETON_TEST_CCONFIG_H
#include <fstream>
#include <map>
#include <linux/input-event-codes.h>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

class CConfig
{
private:
    CConfig() = default;
    CConfig( const CConfig&);
    CConfig& operator=( CConfig& );
    std::ifstream ifs { R"(config.json)" };
    rapidjson::IStreamWrapper isw {ifs};
    rapidjson::Document doc {};
//    std::map<std::string, int> key_map = {
//            {"Tab", KEY_TAB},
//            {"Enter", KEY_ENTER},
//            {"Space", KEY_SPACE},
//            {"Esc", KEY_ESC},
//            {"Backspace", KEY_BACKSPACE},
//            {"NONE", 0}
//    };
public:
    static CConfig& getInstance() {
        static CConfig instance;
        return instance;
    }

    void initConfig();
    std::string getUrl();
    std::string getHotkey();
};


#endif //SINGLETON_TEST_CCONFIG_H
