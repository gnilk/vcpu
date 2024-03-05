//
// Created by gnilk on 15.01.2024.
//

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include <memory>

#include "Interrupt.h"

namespace gnilk {
    namespace vcpu {
        class Peripheral {
        public:
            using Ref = std::shared_ptr<Peripheral>;
        public:
            Peripheral() = default;
            virtual ~Peripheral() = default;

            virtual void Initialize() {}
            virtual bool Update() { return false; }

            void SetInterruptController(InterruptController *newCntrl) {
                intController = newCntrl;
            }
            void MapToInterrupt(CPUInterruptId newInterruptId) {
                interruptId = newInterruptId;
            }
        protected:
            void RaiseInterrupt() {
                if (intController == nullptr) {
                    return;
                }
                intController->RaiseInterrupt(interruptId);
            }
        protected:
            InterruptController *intController =nullptr;
            CPUInterruptId interruptId = {};
        };
    }
}

#endif //PERIPHERAL_H
