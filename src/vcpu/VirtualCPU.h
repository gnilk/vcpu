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
#include "InstructionSetV1/InstructionSetV1Impl.h"
#include "InstructionSetV1/InstructionSetV1Decoder.h"
#include "InstructionSetV1/InstructionSetV1Def.h"
#include "MemorySubSys/MemoryUnit.h"
#include "Timer.h"
#include <array>
#include "Dispatch.h"

namespace gnilk {

    namespace vcpu {


        class VirtualCPU : public CPUBase {
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
            bool ProcessDispatch();

        private:
            Timer *timer0;
            LastInstruction lastDecodedInstruction;
            //InstructionDecoder::Ref lastDecodedInstruction = nullptr;
        };
    }
}



#endif //VIRTUALCPU_H
