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
#include <thread>

#include "VirtualCPU.h"
#include "Timer.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
    DLL_EXPORT int test_timer(ITesting *t);
    DLL_EXPORT int test_timer_ticks1hz(ITesting *t);
    DLL_EXPORT int test_timer_ticks1000hz(ITesting *t);
    DLL_EXPORT int test_timer_startstop(ITesting *t);
    DLL_EXPORT int test_timer_interrupt(ITesting *t);
}

DLL_EXPORT int test_timer(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_timer_ticks1hz(ITesting *t) {
    TimerConfigBlock timerConfig = {
            .control = {
                    .enable = 1,
                    .reset = 1,
            },
            .freqSec = 1,
            .tickCounter = 0,
    };
    auto timer = Timer::Create(&timerConfig);
    timer->Start();

    int lastTick = -1;
    int tc = 0;
    auto tStart = std::chrono::system_clock::now();
    auto dur = std::chrono::system_clock::now() - tStart;
    while(std::chrono::duration_cast<std::chrono::seconds>(dur).count() < 10) {
        if (timerConfig.tickCounter != lastTick) {
            lastTick = timerConfig.tickCounter;
            printf("tick, tc=%d\n", tc);
            tc++;
        }
        dur = std::chrono::system_clock::now() - tStart;
    }
    timer->Stop();
    // We should have 10 ticks, but since the timer is free-standing there can be an extra tick during..
    TR_ASSERT(t, tc >= 10);
    TR_ASSERT(t, tc < 12);

    printf("Total Ticks: %d\n", tc);
    return kTR_Pass;
}

DLL_EXPORT int test_timer_ticks1000hz(ITesting *t) {
    // 1000 ticks per second - this is basically a systick timer..
    TimerConfigBlock timerConfig = {
            .control = {
                    .enable = 1,
                    .reset = 1,
            },
            .freqSec = 1000,
            .tickCounter = 0,
    };
    auto timer = Timer::Create(&timerConfig);
    auto tStart = std::chrono::system_clock::now();
    timer->Start();
    int tc = 0;
    auto dur = std::chrono::system_clock::now() - tStart;
    printf("Running timer for 5 seconds - measuring number of ticks\n");
    while(std::chrono::duration_cast<std::chrono::seconds>(dur).count() < 5) {
        tc++;
        dur = std::chrono::system_clock::now() - tStart;
        std::this_thread::yield();
    }
    printf("Duration=%d\n", (int)std::chrono::duration_cast<std::chrono::seconds>(dur).count());
    printf("ticks, tc=%d, timer.tickCounter=%d\n", tc, (int)timerConfig.tickCounter);

    // Allow some slack - timers aren't exactly super-precise, normally we hit 4995..
    TR_ASSERT(t, (timerConfig.tickCounter > 4900) && (timerConfig.tickCounter < 5100));

    timer->Stop();

    return kTR_Pass;
}
DLL_EXPORT int test_timer_startstop(ITesting *t) {
    // This is a timer when the CPU starts...
    TimerConfigBlock timerConfig = {
            .control = {
                    .enable = 0,
                    .reset = 0,
            },
            .freqSec = 0,
            .tickCounter = 0,
    };
    auto timer = Timer::Create(&timerConfig);

    auto tStart = std::chrono::system_clock::now();
    // Start the synthetic clock
    timer->Start();
    printf("Timer started, waiting one second - see if tick-counter has moved (shouldn't)\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Shouldn't have any updates
    TR_ASSERT(t, timerConfig.tickCounter == 0);

    printf("Configuring it; freq=1000hz, reset=1, enable=1\n");
    printf("Waiting one second, ensure tick-counter has moved\n");


    timerConfig.freqSec = 1000;
    timerConfig.control.reset = 1;
    timerConfig.control.enable = 1;

    // Wait for reset to kick in...
    while(timerConfig.control.running != 1) {
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    TR_ASSERT(t, timerConfig.tickCounter > 0);

    printf("Disable timer, running=%d\n", timerConfig.control.running);
    timerConfig.control.enable = 0;
    // Wait for timer to stop running
    while(timerConfig.control.running != 0) {
        std::this_thread::yield();
    }

    auto tc = timerConfig.tickCounter;
    printf("Sleeping 1 second, tc_before=%d (tick counter shouldn't move)\n",(int)tc);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    TR_ASSERT(t, tc == timerConfig.tickCounter);

    printf("Enable timer again, running=%d\n", timerConfig.control.running);
    timerConfig.control.enable = 1;
    // Wait for timer to stop running
    while(timerConfig.control.running != 1) {
        std::this_thread::yield();
    }
    printf("Wait 1 second, check if tick counter has moved ahead\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    TR_ASSERT(t, tc < timerConfig.tickCounter);

    printf("Done, stopping timer...\n");

    timer->Stop();

    return kTR_Pass;
}

DLL_EXPORT int test_timer_interrupt(ITesting *t) {
    uint8_t program[]= {
        OperandCode::NOP,0x00,0x00,
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    vcpu.Step();
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    vcpu.Step();    // We should have an interrupt tick here!

    return kTR_Pass;
}

