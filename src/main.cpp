#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "config.h"

// ===== LED CONFI =====
#define LED_PIN        48
#define NUM_PIXELS      1
#define LED_ON          5
#define LED_OFF         0
Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
// ===== MQTT CONFIG =====
const char* ssid        = "JXTL-03";
const char* password    = "tinno@805897";
const char* mqtt_server = "8.134.118.221"; 
const int   mqtt_port   = 1883;
const char* mqtt_user   = "esp32_wol";
const char* mqtt_pass   = "qq1742986756";

const char* topic_sub             = "home/action/+";
const char* topic_message_prefix  = "home/message";
// ========================

const char* target_mac; 
int led_color;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiUDP udp;
Config config;

void setLed(uint8_t r, uint8_t g, uint8_t b) {
    led_color = (r << 16) | (g << 8) | b;
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

void setLedEnable(bool enable, bool save = true) {
    strip.setBrightness(enable ? LED_ON : LED_OFF);
    if (save) {
        config.setLedEnable(enable);
    }

    setLed(
        led_color >> 16,
        (led_color >> 8) & 0xff,
        led_color & 0xff 
    );
    if (enable) {
        Serial.printf("[DEVICE] 开启设备LED");
    } else {
        Serial.printf("[DEVICE] 关闭设备LED");
    }
}

void sendWOLPacket(const char* macStr) {
    byte mac[6];
    sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    byte packet[102];
    
    for (int i = 0; i < 6; i++) {
        packet[i] = 0xFF;
    }
    
    for (int i = 1; i <= 16; i++) {
        memcpy(&packet[i * 6], mac, 6);
    }

    udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
    udp.write(packet, sizeof(packet));
    udp.endPacket();

    Serial.println("[WOL] 原生魔法数据包已成功广播发送！");
}

char* getSuffix(char* topic) {
    int ind = 0, len = 0;
    for (int i = 0; topic[i]; ++i) {
        if (topic[i] == '/') {
            ind = i;
        }
        len++;
    }
    char* result = new char[len];
    for (int i = ind, j = 0; i < len; ++i, ++j) {
        result[j] = topic[i];
    }
    return result;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    String topic_str = String(topic);
    String topic_message = String(topic_message_prefix) + topic_str.substring(topic_str.lastIndexOf("/"));
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.printf("[MQTT] 收到主题 [%s] 的消息: %s %s\n", topic, message, topic_message.c_str());
    if (topic_str == "home/action/boot") {
        setLed(0, 0, 255); 
        sendWOLPacket(target_mac);
        mqttClient.publish(topic_message.c_str(), "{}");
        vTaskDelay(pdMS_TO_TICKS(500));
        setLed(0, 255, 0);
    } else if (topic_str == "home/action/status") {
        JsonDocument doc;
        doc["wifi_signal"] = WiFi.RSSI();
        doc["target_mac"] = target_mac;
        doc["light"] = config.getLedEnable();
        char json_buffer[256];
        serializeJson(doc, json_buffer);
        if (mqttClient.publish(topic_message.c_str(), json_buffer)) {
            Serial.printf("[MQTT] 发送成功! 内容: %s\n", json_buffer);
        }
    } else if (topic_str == "home/action/setmac") {
        JsonDocument doc;

        if (config.setMac(message)) {
            target_mac = config.getMac();
            doc["result"] = true;
            Serial.printf("[MQTT] 设置目标 MAC: %s 成功！", message);
        } else {
            doc["result"] = false;
            Serial.printf("[MQTT] 无效的MAC: %s", message);
        }
        doc["target_mac"] = target_mac;
        char json_buffer[256];
        serializeJson(doc, json_buffer);
        mqttClient.publish(topic_message.c_str(), json_buffer);
    } else if (topic_str == "home/action/light") {
        if (message == "on") {
            setLedEnable(true);
        } else if (message == "off") {
            setLedEnable(false);
        }
        mqttClient.publish(topic_message.c_str(), "{}");
    }
}

void MQTTTask(void * pvParameters) {
    strip.begin();
    strip.setBrightness(config.getLedEnable() ? LED_ON : LED_OFF);
    setLed(255, 0, 0);

    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    for(;;) {
        if (WiFi.status() != WL_CONNECTED) {
            setLed(255, 0, 0);
            Serial.println("[MQTT] 尝试连接到WIFI...");
            WiFi.begin(ssid, password);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (!mqttClient.connected()) {
            setLed(255, 0, 0);
            Serial.println("[MQTT] 尝试连接服务器...");
            String clientId = "ESP32S3-WOL-" + String(random(0, 0xffff), HEX);
            
            if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
                Serial.println("成功!");
                mqttClient.subscribe(topic_sub);
                setLed(0, 255, 0);
            } else {
                Serial.printf("失败, 错误码=%d, 2秒后重试\n", mqttClient.state());
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }
        }

        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
const int touchPin = 4;
int touchValue = 0;
void setup() {
    Serial.begin(9600);
    target_mac = config.getMac();
    xTaskCreatePinnedToCore(MQTTTask, "MQTTTask", 4096, NULL, 1, NULL, 0);
}

void loop() {
    touchValue = touchRead(touchPin); 
    Serial.print("触摸读数: ");
    Serial.println(touchValue);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
}