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

namespace gnilk {

    namespace vcpu {

        class VirtualCPU : public CPUBase {
        public:
            VirtualCPU() = default;
            virtual ~VirtualCPU() = default;
            void Begin(void *ptrRam, size_t sizeofRam) {
                ram = static_cast<uint8_t *>(ptrRam);
                szRam = sizeofRam;
                // Everything is zero upon reset...
                memset(&registers, 0, sizeof(registers));
            }
            void Reset() {
                registers.instrPointer.data.longword = 0;
            }
            bool Step();

            const InstructionDecoder::Ref GetLastDecodedInstr() const {
                return lastDecodedInstruction;
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
            void ExecuteLeaInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteLsrInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteLslInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteAslInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteAsrInstr(InstructionDecoder::Ref instrDecoder);


            void WriteToDst(InstructionDecoder::Ref instrDecoder, const RegisterValue &v);
        private:
            InstructionDecoder::Ref lastDecodedInstruction = nullptr;
        };
    }
}



#endif //VIRTUALCPU_H
