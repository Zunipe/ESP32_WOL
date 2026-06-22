#pragma once

namespace AppConfig {

// NeoPixel
constexpr uint8_t LED_PIN = 48;
constexpr uint8_t NUM_PIXELS = 1;
constexpr uint8_t LED_ON_BRIGHTNESS = 5;
constexpr uint8_t LED_OFF_BRIGHTNESS = 0;

// WiFi
constexpr const char* WIFI_SSID = "Xiaomi_4DFF";
constexpr const char* WIFI_PASSWORD = "qq1742986756";

// MQTT
constexpr const char* MQTT_SERVER = "8.134.118.221";
constexpr int MQTT_PORT = 1883;
constexpr const char* MQTT_USER = "esp32_wol";
constexpr const char* MQTT_PASS = "qq1742986756";
constexpr const char* MQTT_TOPIC_SUB = "home/action/+";
constexpr const char* MQTT_TOPIC_MESSAGE_PREFIX = "home/message";

// NTP (UTC+8)
constexpr const char* NTP_SERVER = "ntp.aliyun.com";
constexpr long GMT_OFFSET_SEC = 8 * 3600;
constexpr int DAYLIGHT_OFFSET_SEC = 0;

// Light schedule
constexpr int SCHEDULE_OFF_HOUR = 23;
constexpr int SCHEDULE_ON_HOUR = 8;
constexpr unsigned long SCHEDULE_CHECK_INTERVAL_MS = 60000;

// Task timing
constexpr unsigned long WIFI_RETRY_MS = 2000;
constexpr unsigned long MQTT_RETRY_MS = 2000;
constexpr unsigned long MQTT_LOOP_DELAY_MS = 50;
constexpr unsigned long WOL_LED_HOLD_MS = 500;

}  // namespace AppConfig
