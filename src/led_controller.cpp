#include "led_controller.h"

#include "app_config.h"

LedController::LedController()
    : strip_(AppConfig::NUM_PIXELS, AppConfig::LED_PIN, NEO_GRB + NEO_KHZ800) {}

void LedController::begin() {
    strip_.begin();
    render();
}

void LedController::setColor(uint8_t r, uint8_t g, uint8_t b) {
    color_ = (static_cast<uint32_t>(r) << 16) |
             (static_cast<uint32_t>(g) << 8) |
             static_cast<uint32_t>(b);
    render();
}

void LedController::setPhysicalOn(bool on) {
    if (physicalOn_ == on) {
        return;
    }

    physicalOn_ = on;
    render();
}

bool LedController::isPhysicalOn() const {
    return physicalOn_;
}

void LedController::render() {
    strip_.setBrightness(physicalOn_ ? AppConfig::LED_ON_BRIGHTNESS : AppConfig::LED_OFF_BRIGHTNESS);
    strip_.setPixelColor(
        0,
        strip_.Color(
            (color_ >> 16) & 0xFF,
            (color_ >> 8) & 0xFF,
            color_ & 0xFF));
    strip_.show();
}
