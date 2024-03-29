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
#include "InstructionDecoder.h"
#include "InstructionSet.h"
#include "MemoryUnit.h"
#include "Timer.h"

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

        protected:
            // one operand instr.
            void ExecutePushInstr(InstructionDecoder::Ref instrDecoder);
            void ExecutePopInstr(InstructionDecoder::Ref instrDecoder);

            // two operand instr.
            void ExecuteSysCallInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteMoveInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteAddInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteSubInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteMulInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteDivInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteCallInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteRetInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteRtiInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteLeaInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteLsrInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteLslInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteAslInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteAsrInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteCmpInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteBeqInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteBneInstr(InstructionDecoder::Ref instrDecoder);


            void WriteToDst(InstructionDecoder::Ref instrDecoder, const RegisterValue &v);
        private:
            Timer *timer0;
            LastInstruction lastDecodedInstruction;
            //InstructionDecoder::Ref lastDecodedInstruction = nullptr;
        };
    }
}



#endif //VIRTUALCPU_H
