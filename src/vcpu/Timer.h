//
// Created by gnilk on 15.01.2024.
//

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

#include "Peripheral.h"

namespace gnilk {
    namespace vcpu {


        // This is the memory mapping for the timer...
        struct TimerConfigBlock {
            uint64_t control = {};
            uint64_t freqSec = {};
            uint64_t tickCounter = {};
        };

        class Timer : public Peripheral {
        public:
            using clock = std::chrono::high_resolution_clock;
        public:
            Timer(uint64_t freqHz) : freqSec(freqHz) {

            }
            virtual ~Timer() = default;

            // FIXME: Timer should be created with a pointer to memory block for timer-cfg
            static Ref Create(uint64_t freqHz);

            void Initialize() override;
            bool Update() override;
        private:
            // move this out of here - should be in the 'TimerConfigBlock'
            uint64_t freqSec = 1;   // 32khz is the default.

            // Internal - for emulation//
            bool bHaveFirstTime = false;
            clock::time_point tLast;
        };
    }
}



#endif //TIMER_H
