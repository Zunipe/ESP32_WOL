#pragma once

#include "config.h"

class LedController;

class TimeService {
public:
    bool sync();
    void reset();
    bool isSynced() const;
    bool shouldLightBeOn(const Config& config) const;
    void checkSchedule(const Config& config, LedController& led);

private:
    bool synced_ = false;
    bool scheduleInitialized_ = false;
    bool lastPhysicalOn_ = false;

    bool isInSleepHours() const;
};
