#pragma once

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

class LedController {
public:
    LedController();

    void begin();
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setPhysicalOn(bool on);
    bool isPhysicalOn() const;

private:
    Adafruit_NeoPixel strip_;
    uint32_t color_ = 0xFF0000;
    bool physicalOn_ = false;

    void render();
};
