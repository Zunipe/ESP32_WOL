#pragma once
#include <Preferences.h>

class Config {
public:
    char* getMac();
    bool setMac(const String mac);
    bool getLedEnable();
    void setLedEnable(const bool enable);
private:
    Preferences prefs;
    const char* config = "Config";
};