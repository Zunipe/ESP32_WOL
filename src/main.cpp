#include <Arduino.h>

#include "config.h"
#include "led_controller.h"
#include "mqtt_app.h"
#include "time_sync.h"

namespace {

Config config;
LedController led;
TimeService timeService;
MqttApp mqttApp(config, led, timeService);

}  // namespace

void setup() {
    Serial.begin(9600);
    mqttApp.begin();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
