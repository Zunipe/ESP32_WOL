#pragma once
#include <Preferences.h>

class Config {
public:
    char* getMac();
    bool setMac(const String mac);
private:
    Preferences prefs;
    const char* config = "Config";
};