//
// Created by gnilk on 27.03.24.
//

#ifndef VCPU_INSTRUCTIONSETV1IMPL_H
#define VCPU_INSTRUCTIONSETV1IMPL_H

#include "CPUBase.h"
#include "InstructionDecoderBase.h"
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
            void ExecutePushInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecutePopInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteSysCallInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);

            // two operand instr.
            void ExecuteMoveInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteAddInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteSubInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteMulInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteDivInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteCallInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteRetInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteRtiInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteRteInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteLeaInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteLsrInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteLslInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteAslInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteAsrInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteCmpInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteBeqInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);
            void ExecuteBneInstr(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder);

            void ExecuteSIMDInstr(CPUBase &cpu, InstructionDecoderBase &baseDecoder);
            void WriteToDst(CPUBase &cpu, InstructionSetV1Decoder& instrDecoder, const RegisterValue &v);

        };
    }
}


#endif //VCPU_INSTRUCTIONSETV1IMPL_H
