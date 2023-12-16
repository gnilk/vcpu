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

namespace gnilk {

    namespace vcpu {
        class IMemReadWrite {
        public:
            virtual uint8_t  ReadU8(uint64_t address) = 0;
            virtual uint16_t ReadU16(uint64_t address) = 0;
            virtual uint32_t ReadU32(uint64_t address) = 0;
            virtual uint64_t ReadU64(uint64_t address) = 0;

            virtual void WriteU8(uint64_t address, uint8_t value) = 0;
            virtual void WriteU16(uint64_t address, uint16_t value) = 0;
            virtual void WriteU32(uint64_t address, uint32_t value) = 0;
            virtual void WriteU64(uint64_t address, uint64_t value) = 0;
        };





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

        protected:
            // one operand instr.
            void ExecutePushInstr(InstructionDecoder::Ref instrDecoder);
            void ExecutePopInstr(InstructionDecoder::Ref instrDecoder);

            // two operand instr.
            void ExecuteMoveInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteAddInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteSubInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteMulInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteDivInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteCallInstr(InstructionDecoder::Ref instrDecoder);
            void ExecuteRetInstr(InstructionDecoder::Ref instrDecoder);

            // Perhaps move to base class
            RegisterValue ReadFrom(OperandSize szOperand, AddressMode addrMode, int idxRegister);
            RegisterValue ReadImmediateMode(OperandSize szOperand);

            void WriteToDst(InstructionDecoder::Ref instrDecoder, const RegisterValue &v);


        };
    }
}



#endif //VIRTUALCPU_H
