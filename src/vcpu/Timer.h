//
// Created by gnilk on 15.01.2024.
//

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

#include <mutex>
#include "Peripheral.h"
#include <thread>

namespace gnilk {
    namespace vcpu {


        struct TimerControl {
            uint8_t enable : 1;
            uint8_t reset : 1;
            uint8_t running : 1;
        };
        // This is the memory mapping for the timer...
        struct TimerConfigBlock {
            union {
                TimerControl control;
                uint64_t controlBits = {};
            };
            uint64_t freqSec = {};
            uint64_t tickCounter = {};
        };

        class Timer : public Peripheral {
        public:
            using clock = std::chrono::high_resolution_clock;
        public:
            Timer(TimerConfigBlock *ptrConfigBlock) : config(ptrConfigBlock) {

            }
            virtual ~Timer();

            // FIXME: Timer should be created with a pointer to memory block for timer-cfg
            static Ref Create(uint64_t freqHz);

            static Ref Create(TimerConfigBlock *ptrConfigBlock);

            bool Start() override;
            bool Stop() override;
        protected:
            bool DoStop();
            void ThreadFunc();
            void CountTicks();
        private:
            TimerConfigBlock *config = nullptr;
            bool bStopTimer = false;    // make atomic...
            std::thread timerThread;
            std::mutex lock;

            // move this out of here - should be in the 'TimerConfigBlock'
            uint64_t freqSec = 1;   // 32khz is the default.
            double microsPerTick = {};
            // Internal - for emulation//
            bool bHaveFirstTime = false;
            clock::time_point tLast;
        };
    }
}



#endif //TIMER_H
