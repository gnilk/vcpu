//
// Created by gnilk on 15.01.2024.
//

#include <chrono>
#include "Timer.h"

using namespace gnilk;
using namespace gnilk::vcpu;

Peripheral::Ref Timer::Create(uint64_t freqHz) {
    auto t = std::make_shared<Timer>(freqHz);
    return t;
}

void Timer::Initialize() {
    auto tLast = clock::now();
}

bool Timer::Update() {
    bool res = false;

    auto tNow = clock::now();

    auto dur = tNow - tLast;
    auto microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    auto microsPerTick =  1000 * 1000 * (double)(1.0 / freqSec);
    if (microSeconds > microsPerTick) {
        // raise timer interrupt..
        RaiseInterrupt();
        res = true;
        tLast = tNow;
    }
    return res;
}
