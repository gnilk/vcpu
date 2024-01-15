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
            void MapToInterrupt(CPUISRType isrType) {
                assignedIsr = isrType;
            }
        protected:
            void RaiseInterrupt() {
                if (intController == nullptr) {
                    return;
                }
                intController->RaiseInterrupt(assignedIsr);
            }
        protected:
            InterruptController *intController =nullptr;
            CPUISRType assignedIsr = CPUISRType::ISRNone;
        };
    }
}

#endif //PERIPHERAL_H
