#include "time_sync.h"

#include <time.h>

#include "app_config.h"
#include "led_controller.h"

bool TimeService::sync() {
    configTime(AppConfig::GMT_OFFSET_SEC, AppConfig::DAYLIGHT_OFFSET_SEC, AppConfig::NTP_SERVER);

    struct tm timeinfo;
    for (int i = 0; i < 10; i++) {
        if (getLocalTime(&timeinfo)) {
            synced_ = true;
            Serial.printf(
                "[NTP] 时间同步成功: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    synced_ = false;
    Serial.println("[NTP] 时间同步失败");
    return false;
}

void TimeService::reset() {
    synced_ = false;
    scheduleInitialized_ = false;
}

bool TimeService::isSynced() const {
    return synced_;
}

bool TimeService::isInSleepHours() const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }

    return timeinfo.tm_hour >= AppConfig::SCHEDULE_OFF_HOUR ||
           timeinfo.tm_hour < AppConfig::SCHEDULE_ON_HOUR;
}

bool TimeService::shouldLightBeOn(const Config& config) const {
    if (!config.getLedEnable()) {
        return false;
    }
    if (!synced_) {
        return true;
    }
    return !isInSleepHours();
}

void TimeService::checkSchedule(const Config& config, LedController& led) {
    if (!synced_) {
        return;
    }

    const bool physicalOn = shouldLightBeOn(config);
    if (!scheduleInitialized_) {
        scheduleInitialized_ = true;
        lastPhysicalOn_ = physicalOn;
        led.setPhysicalOn(physicalOn);
        return;
    }

    if (physicalOn == lastPhysicalOn_) {
        return;
    }

    lastPhysicalOn_ = physicalOn;
    led.setPhysicalOn(physicalOn);
    Serial.printf("[SCHEDULE] 定时调整灯光: %s\n", physicalOn ? "开启" : "关闭");
}
