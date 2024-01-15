//
// Created by gnilk on 15.01.2024.
//

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

#include "Peripheral.h"

namespace gnilk {
    namespace vcpu {
        class Timer : public Peripheral {
        public:
            using clock = std::chrono::high_resolution_clock;
        public:
            Timer() = default;
            virtual ~Timer() = default;

            void Update() override;
        private:
            int freq = 32000;   // 32khz is the default.
            bool bHaveFirstTime = false;
            clock::time_point tLast;
        };
    }
}



#endif //TIMER_H
