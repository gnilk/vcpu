//
// Created by gnilk on 15.01.2024.
//

#include <chrono>
#include "Timer.h"

using namespace gnilk;
using namespace gnilk::vcpu;

void Timer::Update() {
    auto tNow = clock::now();
    if (!bHaveFirstTime) {
        tLast = tNow;
        return;
    }
    auto dur = tNow - tLast;
    auto microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    auto microsPerTick = freq / 1000;
    if (microSeconds > microsPerTick) {
        // raise tick..

        tLast = tNow;
    }
}
