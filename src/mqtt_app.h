#pragma once

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"
#include "led_controller.h"
#include "time_sync.h"

class MqttApp {
public:
    MqttApp(Config& config, LedController& led, TimeService& time);

    void begin();

private:
    Config& config_;
    LedController& led_;
    TimeService& time_;
    String targetMac_;

    WiFiClient wifiClient_;
    PubSubClient mqttClient_;
    bool ntpInitialized_ = false;
    unsigned long lastScheduleCheckMs_ = 0;

    void applyLightState();
    void setUserLightEnabled(bool enable);
    void handleMessage(char* topic, byte* payload, unsigned int length);
    void publishJson(const String& replyTopic, JsonDocument& doc);
    void run();

    static void taskEntry(void* param);
    static void messageCallback(char* topic, byte* payload, unsigned int length);

    static MqttApp* instance_;
};
