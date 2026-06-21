#include "config.h"
#include <regex>

bool isVaildMac(const String mac);

char* Config::getMac() {
    prefs.begin(config, true);
    char* mac = new char[18];
    prefs.getString("mac", "00:E2:69:90:11:E7").toCharArray(mac, 18);
    prefs.end();
    return mac;
}

bool Config::setMac(const String mac) {
    prefs.begin(config, false);
    if (!isVaildMac(mac)) {
        return false;
    }
    prefs.putString("mac", mac.c_str());
    prefs.end();
    return true;
}

bool isVaildMac(const String mac) {
    String p = "nn:nn:nn:nn:nn:nn";
    if (mac.length() != p.length()) {
        return false;
    }

    for (int i = 0; i < p.length(); ++i) {
        if (p[i] == 'n') {
            if (mac[i] >= '0' && mac[i] <= '9') {
                continue;
            }
            if (mac[i] >= 'A' && mac[i] <= 'F') {
                continue;
            }
            if (mac[i] >= 'a' && mac[i] <= 'f') {
                continue;
            }
            return false;
        } else if(mac[i] != ':') {
            return false;
        }
    }
    return true;
}