#include "config.h"

String Config::getMac() const {
    prefs_.begin(NAMESPACE, true);
    String mac = prefs_.getString(KEY_MAC, DEFAULT_MAC);
    prefs_.end();
    return mac;
}

bool Config::setMac(const String& mac) {
    if (!isValidMac(mac)) {
        return false;
    }

    prefs_.begin(NAMESPACE, false);
    prefs_.putString(KEY_MAC, mac);
    prefs_.end();
    return true;
}

bool Config::getLedEnable() const {
    prefs_.begin(NAMESPACE, true);
    bool enable = prefs_.getBool(KEY_LED_ENABLE, true);
    prefs_.end();
    return enable;
}

void Config::setLedEnable(bool enable) {
    prefs_.begin(NAMESPACE, false);
    prefs_.putBool(KEY_LED_ENABLE, enable);
    prefs_.end();
}

bool isValidMac(const String& mac) {
    if (mac.length() != 17) {
        return false;
    }

    for (int i = 0; i < 17; ++i) {
        const char c = mac[i];
        if (i % 3 == 2) {
            if (c != ':') {
                return false;
            }
            continue;
        }

        const bool isDigit = c >= '0' && c <= '9';
        const bool isUpperHex = c >= 'A' && c <= 'F';
        const bool isLowerHex = c >= 'a' && c <= 'f';
        if (!isDigit && !isUpperHex && !isLowerHex) {
            return false;
        }
    }

    return true;
}
