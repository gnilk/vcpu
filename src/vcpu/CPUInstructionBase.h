//
// Created by gnilk on 27.03.24.
//

#ifndef VCPU_CPUINSTRUCTIONBASE_H
#define VCPU_CPUINSTRUCTIONBASE_H

#include "CPUBase.h"
#include "InstructionDecoder.h"

namespace gnilk {
    namespace vcpu {
        // FIXME: This should handled through composition, supply the CPUBase as an argument instead
        //        Would allow swapping instructions sets...
        class CPUInstructionBase : public CPUBase {
        public:
            virtual bool ExecuteInstruction(InstructionDecoder &decoder);

        protected:
            // one operand instr.
            void ExecutePushInstr(InstructionDecoder& instrDecoder);
            void ExecutePopInstr(InstructionDecoder& instrDecoder);

            // two operand instr.
            void ExecuteSysCallInstr(InstructionDecoder& instrDecoder);
            void ExecuteMoveInstr(InstructionDecoder& instrDecoder);
            void ExecuteAddInstr(InstructionDecoder& instrDecoder);
            void ExecuteSubInstr(InstructionDecoder& instrDecoder);
            void ExecuteMulInstr(InstructionDecoder& instrDecoder);
            void ExecuteDivInstr(InstructionDecoder& instrDecoder);
            void ExecuteCallInstr(InstructionDecoder& instrDecoder);
            void ExecuteRetInstr(InstructionDecoder& instrDecoder);
            void ExecuteRtiInstr(InstructionDecoder& instrDecoder);
            void ExecuteLeaInstr(InstructionDecoder& instrDecoder);
            void ExecuteLsrInstr(InstructionDecoder& instrDecoder);
            void ExecuteLslInstr(InstructionDecoder& instrDecoder);
            void ExecuteAslInstr(InstructionDecoder& instrDecoder);
            void ExecuteAsrInstr(InstructionDecoder& instrDecoder);
            void ExecuteCmpInstr(InstructionDecoder& instrDecoder);
            void ExecuteBeqInstr(InstructionDecoder& instrDecoder);
            void ExecuteBneInstr(InstructionDecoder& instrDecoder);


            void WriteToDst(InstructionDecoder& instrDecoder, const RegisterValue &v);

        };
    }
}


#endif //VCPU_CPUINSTRUCTIONBASE_H
