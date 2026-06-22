#include "wol.h"

#include <WiFiUdp.h>

namespace {

WiFiUDP udp;

}  // namespace

bool sendWolPacket(const char* macStr) {
    uint8_t mac[6];
    if (sscanf(
            macStr,
            "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0],
            &mac[1],
            &mac[2],
            &mac[3],
            &mac[4],
            &mac[5]) != 6) {
        Serial.println("[WOL] MAC 地址格式无效");
        return false;
    }

    uint8_t packet[102];
    memset(packet, 0xFF, 6);
    for (int i = 1; i <= 16; i++) {
        memcpy(&packet[i * 6], mac, 6);
    }

    if (!udp.beginPacket(IPAddress(255, 255, 255, 255), 9)) {
        Serial.println("[WOL] 无法创建 UDP 数据包");
        return false;
    }

    udp.write(packet, sizeof(packet));
    if (!udp.endPacket()) {
        Serial.println("[WOL] 发送失败");
        return false;
    }

    Serial.println("[WOL] 魔法数据包已成功广播发送");
    return true;
}
