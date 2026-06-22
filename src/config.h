#pragma once

#include <Arduino.h>
#include <Preferences.h>

class Config {
public:
    String getMac() const;
    bool setMac(const String& mac);
    bool getLedEnable() const;
    void setLedEnable(bool enable);

private:
    mutable Preferences prefs_;
    static constexpr const char* NAMESPACE = "Config";
    static constexpr const char* KEY_MAC = "mac";
    static constexpr const char* KEY_LED_ENABLE = "led_enable";
    static constexpr const char* DEFAULT_MAC = "00:E2:69:90:11:E7";
};

bool isValidMac(const String& mac);
