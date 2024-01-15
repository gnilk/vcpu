//
// Created by gnilk on 15.01.2024.
//
//
// Created by gnilk on 14.12.23.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>
#include <chrono>

#include "Timer.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
    DLL_EXPORT int test_timer(ITesting *t);
    DLL_EXPORT int test_timer_ticks1hz(ITesting *t);
    DLL_EXPORT int test_timer_ticks1000hz(ITesting *t);
}

DLL_EXPORT int test_timer(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_timer_ticks1hz(ITesting *t) {
    Timer timer(1);
    auto tStart = std::chrono::system_clock::now();
    timer.Initialize();
    int tc = 0;
    auto dur = std::chrono::system_clock::now() - tStart;
    while(std::chrono::duration_cast<std::chrono::seconds>(dur).count() < 10) {
        if (timer.Update()) {
            printf("tick, tc=%d\n", tc);
            tc++;
        }
        dur = std::chrono::system_clock::now() - tStart;
    }
    TR_ASSERT(t, tc == 10);
    return kTR_Pass;
}

DLL_EXPORT int test_timer_ticks1000hz(ITesting *t) {
    // 1000 ticks per second - this is basically a systick timer..
    Timer timer(1000);
    auto tStart = std::chrono::system_clock::now();
    timer.Initialize();
    int tc = 0;
    auto dur = std::chrono::system_clock::now() - tStart;
    while(std::chrono::duration_cast<std::chrono::seconds>(dur).count() < 5) {
        if (timer.Update()) {
            tc++;
        }
        dur = std::chrono::system_clock::now() - tStart;
    }
    printf("Duration=%d\n", (int)std::chrono::duration_cast<std::chrono::seconds>(dur).count());
    printf("ticks, tc=%d\n", tc);
    TR_ASSERT(t, (tc > 4900) && (tc < 5100));
    return kTR_Pass;
}
