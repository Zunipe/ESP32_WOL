#include "mqtt_app.h"

#include <ArduinoJson.h>
#include <time.h>

#include "app_config.h"
#include "wol.h"

MqttApp* MqttApp::instance_ = nullptr;

MqttApp::MqttApp(Config& config, LedController& led, TimeService& time)
    : config_(config),
      led_(led),
      time_(time),
      mqttClient_(wifiClient_) {}

void MqttApp::begin() {
    instance_ = this;
    targetMac_ = config_.getMac();

    xTaskCreatePinnedToCore(taskEntry, "MQTTTask", 4096, this, 1, nullptr, 0);
}

void MqttApp::applyLightState() {
    led_.setPhysicalOn(time_.shouldLightBeOn(config_));
}

void MqttApp::setUserLightEnabled(bool enable) {
    config_.setLedEnable(enable);
    applyLightState();
    Serial.printf("[DEVICE] %s设备LED\n", enable ? "开启" : "关闭");
}

void MqttApp::publishJson(const String& replyTopic, JsonDocument& doc) {
    char jsonBuffer[384];
    serializeJson(doc, jsonBuffer);
    if (mqttClient_.publish(replyTopic.c_str(), jsonBuffer)) {
        Serial.printf("[MQTT] 发送成功! 内容: %s\n", jsonBuffer);
    }
}

void MqttApp::handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        message += static_cast<char>(payload[i]);
    }

    const String topicStr(topic);
    const String replyTopic =
        String(AppConfig::MQTT_TOPIC_MESSAGE_PREFIX) +
        topicStr.substring(topicStr.lastIndexOf('/'));

    Serial.printf("[MQTT] 收到主题 [%s] 的消息: %s\n", topic, message.c_str());

    if (topicStr == "home/action/boot") {
        led_.setColor(0, 0, 255);
        sendWolPacket(targetMac_.c_str());
        mqttClient_.publish(replyTopic.c_str(), "{}");
        vTaskDelay(pdMS_TO_TICKS(AppConfig::WOL_LED_HOLD_MS));
        led_.setColor(0, 255, 0);
        return;
    }

    if (topicStr == "home/action/status") {
        JsonDocument doc;
        doc["wifi_signal"] = WiFi.RSSI();
        doc["target_mac"] = targetMac_;
        doc["light"] = config_.getLedEnable();
        doc["light_on"] = led_.isPhysicalOn();
        doc["time_synced"] = time_.isSynced();

        if (time_.isSynced()) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                char timeBuffer[32];
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
                doc["local_time"] = timeBuffer;
            }
        }

        publishJson(replyTopic, doc);
        return;
    }

    if (topicStr == "home/action/setmac") {
        JsonDocument doc;
        if (config_.setMac(message)) {
            targetMac_ = config_.getMac();
            doc["result"] = true;
            Serial.printf("[MQTT] 设置目标 MAC: %s 成功\n", message.c_str());
        } else {
            doc["result"] = false;
            Serial.printf("[MQTT] 无效的 MAC: %s\n", message.c_str());
        }
        doc["target_mac"] = targetMac_;
        publishJson(replyTopic, doc);
        return;
    }

    if (topicStr == "home/action/light") {
        if (message == "on") {
            setUserLightEnabled(true);
        } else if (message == "off") {
            setUserLightEnabled(false);
        }
        mqttClient_.publish(replyTopic.c_str(), "{}");
    }
}

void MqttApp::run() {
    led_.begin();
    applyLightState();
    led_.setColor(255, 0, 0);

    mqttClient_.setServer(AppConfig::MQTT_SERVER, AppConfig::MQTT_PORT);
    mqttClient_.setCallback(messageCallback);

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            ntpInitialized_ = false;
            time_.reset();
            led_.setColor(255, 0, 0);
            Serial.println("[MQTT] 尝试连接到 WiFi...");
            WiFi.begin(AppConfig::WIFI_SSID, AppConfig::WIFI_PASSWORD);
            vTaskDelay(pdMS_TO_TICKS(AppConfig::WIFI_RETRY_MS));
            continue;
        }

        if (!ntpInitialized_) {
            if (time_.sync()) {
                applyLightState();
            }
            ntpInitialized_ = true;
        }

        const unsigned long now = millis();
        if (now - lastScheduleCheckMs_ >= AppConfig::SCHEDULE_CHECK_INTERVAL_MS) {
            lastScheduleCheckMs_ = now;
            time_.checkSchedule(config_, led_);
        }

        if (!mqttClient_.connected()) {
            led_.setColor(255, 0, 0);
            Serial.println("[MQTT] 尝试连接服务器...");

            const String clientId = "ESP32S3-WOL-" + String(random(0, 0xFFFF), HEX);
            if (mqttClient_.connect(
                    clientId.c_str(),
                    AppConfig::MQTT_USER,
                    AppConfig::MQTT_PASS)) {
                Serial.println("[MQTT] 连接成功");
                mqttClient_.subscribe(AppConfig::MQTT_TOPIC_SUB);
                led_.setColor(0, 255, 0);
            } else {
                Serial.printf("[MQTT] 连接失败, 错误码=%d\n", mqttClient_.state());
                vTaskDelay(pdMS_TO_TICKS(AppConfig::MQTT_RETRY_MS));
                continue;
            }
        }

        mqttClient_.loop();
        vTaskDelay(pdMS_TO_TICKS(AppConfig::MQTT_LOOP_DELAY_MS));
    }
}

void MqttApp::taskEntry(void* param) {
    static_cast<MqttApp*>(param)->run();
}

void MqttApp::messageCallback(char* topic, byte* payload, unsigned int length) {
    if (instance_) {
        instance_->handleMessage(topic, payload, length);
    }
}
