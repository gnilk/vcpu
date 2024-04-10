//
// Created by gnilk on 27.03.24.
//

#ifndef VCPU_INSTRUCTIONSETIMPL_H
#define VCPU_INSTRUCTIONSETIMPL_H

#include "CPUBase.h"
#include "InstructionDecoder.h"

namespace gnilk {
    namespace vcpu {

        // This is currently not used, but the idea is to have multiple instruction set's in the future..
        // not sure I will ever bother though...
        class InstructionSetImplBase {
        public:
            virtual bool ExecuteInstruction(InstructionDecoder &decoder) = 0;
        };

        // Implements the one and only instruction set
        // Note: the instruction decoder is heavily coupled to the instruction set at this point
        class InstructionSetImpl : public InstructionSetImplBase {
        public:
            InstructionSetImpl(CPUBase &newCpu) : cpu(newCpu) {

            }
            InstructionSetImpl(InstructionSetImpl &other) : InstructionSetImpl(other.cpu) {

            }
            InstructionSetImpl(InstructionSetImpl &&other) noexcept : InstructionSetImpl(other.cpu) {

            }
            InstructionSetImpl &operator = (InstructionSetImpl &&other) noexcept {
                cpu = other.cpu;
                return *this;
            }

            bool ExecuteInstruction(InstructionDecoder &decoder) override;

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
        private:
            CPUBase &cpu;

        };
    }
}


#endif //VCPU_INSTRUCTIONSETIMPL_H
