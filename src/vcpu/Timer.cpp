//
// Created by gnilk on 15.01.2024.
//

#include <chrono>
#include "Timer.h"
#include <thread>

using namespace gnilk;
using namespace gnilk::vcpu;

Timer::~Timer() {
    DoStop();
}

Peripheral::Ref Timer::Create(TimerConfigBlock *ptrConfigBlock) {
    auto t = std::make_shared<Timer>(ptrConfigBlock);
    return t;
}

bool Timer::Start() {
    if (config == nullptr) {
        return false;
    }
    bStopTimer = false;

    auto t = std::thread([this](){
        ThreadFunc();
    });

    timerThread = std::move(t);
    return true;
}

bool Timer::Stop() {
    return DoStop();
}

// We call 'Stop' from DTOR - shouldn't call virtual functions from there...
bool Timer::DoStop() {
    if (!timerThread.joinable()) {
        return false;
    }

    // Disable the timer..
    {
        std::lock_guard<std::mutex> guard(lock);
        if (config != nullptr) {
            config->control.enable = false;
        }
    }

    bStopTimer = true;
    timerThread.join();
    return true;
}

void Timer::ThreadFunc() {
    tLast = clock::now();

    while(!bStopTimer) {
        bool bIsEnabled = false;
        {
            std::lock_guard<std::mutex> guard(lock);
            if (config->control.reset) {
                config->tickCounter = 0;
                config->control.reset = 0;

                // In order to change frequency you need to 'reset' the clock
                freqSec = config->freqSec;

                microsPerTick =  1000.0 * 1000.0 * (1.0 / static_cast<double>(freqSec));

            }
            bIsEnabled = config->control.enable;

            // Copy this over as the state...
            config->control.running = config->control.enable;
        }


        if (bIsEnabled) {
            CountTicks();
        }
        std::this_thread::yield();
    }
}

// Count the ticks
void Timer::CountTicks() {

    auto tNow = clock::now();

    auto dur = tNow - tLast;
    auto microSeconds = (double)std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

    if (microSeconds > microsPerTick) {

        // Auto release lock section; touching config
        {
            std::lock_guard<std::mutex> guard(lock);
            if (config != nullptr) {
                config->tickCounter++;
            }
        }

        // raise timer interrupt..
        RaiseInterrupt();
        tLast = tNow;
    }
}
