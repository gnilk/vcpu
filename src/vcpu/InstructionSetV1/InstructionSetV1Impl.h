//
// Created by gnilk on 27.03.24.
//

#ifndef VCPU_INSTRUCTIONSETV1IMPL_H
#define VCPU_INSTRUCTIONSETV1IMPL_H

#include "CPUBase.h"
#include "InstructionSetV1/InstructionDecoder.h"
#include "InstructionSetImplBase.h"

namespace gnilk {
    namespace vcpu {


        // Implements the one and only instruction set
        // Note: the instruction decoder is heavily coupled to the instruction set at this point
        class InstructionSetV1Impl : public InstructionSetImplBase {
        public:
            // This more like the 'CPUCore'
//            InstructionSetV1Impl(CPUBase &newCpu) : cpu(newCpu) {
//
//            }
//            InstructionSetV1Impl(InstructionSetV1Impl &other) : InstructionSetV1Impl(other.cpu) {
//
//            }
//            InstructionSetV1Impl(InstructionSetV1Impl &&other) noexcept : InstructionSetV1Impl(other.cpu) {
//
//            }

            bool ExecuteInstruction(CPUBase &newCpu, InstructionDecoderBase &baseDecoder) override;

        protected:
            // one operand instr.
            void ExecutePushInstr(CPUBase &cpu,InstructionDecoder& instrDecoder);
            void ExecutePopInstr(CPUBase &cpu,InstructionDecoder& instrDecoder);
            void ExecuteSysCallInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);

            // two operand instr.
            void ExecuteMoveInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteAddInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteSubInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteMulInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteDivInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteCallInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteRetInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteRtiInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteRteInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteLeaInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteLsrInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteLslInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteAslInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteAsrInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteCmpInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteBeqInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);
            void ExecuteBneInstr(CPUBase &cpu, InstructionDecoder& instrDecoder);

            void ExecuteSIMDInstr(CPUBase &cpu, InstructionDecoderBase &baseDecoder);
            void WriteToDst(CPUBase &cpu, InstructionDecoder& instrDecoder, const RegisterValue &v);

        };
    }
}


#endif //VCPU_INSTRUCTIONSETV1IMPL_H
