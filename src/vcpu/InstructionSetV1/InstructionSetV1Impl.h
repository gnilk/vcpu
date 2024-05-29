//
// Created by gnilk on 27.03.24.
//

#ifndef VCPU_INSTRUCTIONSETV1IMPL_H
#define VCPU_INSTRUCTIONSETV1IMPL_H

#include "CPUBase.h"
#include "InstructionSetImplBase.h"
#include "InstructionSetV1Decoder.h"

namespace gnilk {
    namespace vcpu {


        // Implements the one and only instruction set
        // Note: the instruction decoder is heavily coupled to the instruction set at this point
        class InstructionSetV1Impl : public InstructionSetImplBase {
        public:
            bool ExecuteInstruction(CPUBase &newCpu) override;
            std::string DisasmLastInstruction();

        protected:
            // one operand instr.
            void ExecutePushInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecutePopInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteSysCallInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);

            // two operand instr.
            void ExecuteMoveInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteAddInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteSubInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteMulInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteDivInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteCallInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteRetInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteRtiInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteRteInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteLeaInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteLsrInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteLslInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteAslInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteAsrInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteCmpInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteBeqInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);
            void ExecuteBneInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput);

            void WriteToDst(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput, const RegisterValue &v);

        private:
            InstructionSetV1Def::DecoderOutput decoderOutput;

        };
    }
}


#endif //VCPU_INSTRUCTIONSETV1IMPL_H
