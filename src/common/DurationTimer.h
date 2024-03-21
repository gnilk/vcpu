#pragma once

#if defined(WIN32)
#include "platform.h"
#include <winnt.h>
#endif
#include <stdint.h>
#include <chrono>

namespace gnilk {

    class DurationTimer {
    public:
        using Clock = std::chrono::high_resolution_clock;
    public:
        DurationTimer();
        ~DurationTimer() = default;
        void Reset();
        // returns time since reset as seconds...
        double Sample();
    private:
        Clock::time_point  time_start = {};
    };
}