//
// Created by gnilk on 14.12.23.
//

#ifndef VIRTUALCPU_H
#define VIRTUALCPU_H

#include <stdint.h>
#include <stdlib.h>
#include <stack>
#include "fmt/format.h"
#include "CPUBase.h"
#include "CPUInstructionBase.h"
#include "InstructionDecoder.h"
#include "InstructionSet.h"
#include "MemoryUnit.h"
#include "Timer.h"
#include <array>

namespace gnilk {

    namespace vcpu {


        class VirtualCPU : public CPUInstructionBase {
        public:
            VirtualCPU() = default;
            virtual ~VirtualCPU() = default;
            void QuickStart(void *ptrRam, size_t sizeOfRam) override;
            void Begin(void *ptrRam, size_t sizeOfRam) override;
            void Reset() override;

            bool Step();

            const LastInstruction *GetLastDecodedInstr() const {
                return &lastDecodedInstruction;
            }

        private:
            Timer *timer0;
            LastInstruction lastDecodedInstruction;
            //InstructionDecoder::Ref lastDecodedInstruction = nullptr;
        };
    }
}



#endif //VIRTUALCPU_H
