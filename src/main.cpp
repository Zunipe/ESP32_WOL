#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "config.h"

// 1. 硬件与网络配置
#define LED_PIN        48
#define NUM_PIXELS      1
Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "Xiaomi_4DFF";           // 替换为你的WiFi名称
const char* password = "qq1742986756";   // 替换为你的WiFi密码
const char* mqtt_server = "8.134.118.221"; 
const int   mqtt_port   = 1883;
const char* mqtt_user = "esp32_wol";         // 刚才在 EMQX 后台创建的用户名
const char* mqtt_pass = "qq1742986756"; // 刚才在 EMQX 后台创建的密码

const char* topic_sub     = "home/action/+";
const char* topic_status  = "home/message/status";
const char* topic_boot    = "home/message/boot";
const char* topic_setmac    = "home/message/setmac";

// 目标电脑的有线网卡 MAC 地址（注意：中间用冒号隔开）
const char* target_mac; 

// 2. 实例化网络对象
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiUDP udp;
Config config;

// 灯光快捷控制
void setLed(uint8_t r, uint8_t g, uint8_t b) {
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

void sendWOLPacket(const char* macStr) {
    byte mac[6];
    // 解析 MAC 地址字符串到字节数组
    sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    byte packet[102];
    
    // 1. 前 6 个字节为 0xFF
    for (int i = 0; i < 6; i++) {
        packet[i] = 0xFF;
    }
    
    // 2. 紧接着重复 16 次 MAC 地址
    for (int i = 1; i <= 16; i++) {
        memcpy(&packet[i * 6], mac, 6);
    }

    // 3. 通过 UDP 广播发送（向 255.255.255.255 的 9 号端口发送）
    udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
    udp.write(packet, sizeof(packet));
    udp.endPacket();
    
    Serial.println("[WOL] 原生魔法数据包已成功广播发送！");
}

// 3. MQTT 收到消息的回调函数
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    String topicStr = String(topic);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.printf("[MQTT] 收到主题 [%s] 的消息: %s\n", topic, message.c_str());
    if (topicStr == "home/action/boot") {
        setLed(0, 0, 255); 
        sendWOLPacket(target_mac);
        mqttClient.publish(topic_boot, "{}");
        vTaskDelay(pdMS_TO_TICKS(500));
        setLed(0, 255, 0);
    } else if (topicStr == "home/action/status") {
        JsonDocument doc;
        doc["wifi_signal"] = WiFi.RSSI();
        doc["target_mac"] = target_mac;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        if (mqttClient.publish(topic_status, jsonBuffer)) {
            Serial.printf("[MQTT] 发送成功! 内容: %s\n", jsonBuffer);
        }
    } else if (topicStr == "home/action/setmac") {
        JsonDocument doc;

        if (config.setMac(message)) {
            target_mac = config.getMac();
            doc["result"] = true;
            Serial.printf("[MQTT] 设置目标 MAC: %s 成功！", message.c_str());
        } else {
            doc["result"] = false;
            Serial.printf("[MQTT] 无效的MAC: %s", message.c_str());
        }
        doc["target_mac"] = target_mac;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        mqttClient.publish(topic_setmac, jsonBuffer);
    }
}

// 4. 后台 MQTT 与 Wi-Fi 管理任务 (运行在 Core 0)
void MQTTTask(void * pvParameters) {
    strip.begin();
    strip.setBrightness(5);
    setLed(255, 0, 0); // 初始红灯

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
                setLed(0, 255, 0); // 成功变绿灯
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

void setup() {
    Serial.begin(9600);
    target_mac = config.getMac();

    xTaskCreatePinnedToCore(MQTTTask, "MQTTTask", 4096, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000)); 
}