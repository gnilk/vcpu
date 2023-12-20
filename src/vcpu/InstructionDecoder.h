//
// Created by gnilk on 16.12.23.
//

#ifndef INSTRUCTIONDECODER_H
#define INSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "InstructionSet.h"
#include "CPUBase.h"

namespace gnilk {
    namespace vcpu {

        class InstructionDecoder {
        public:
            using Ref = std::shared_ptr<InstructionDecoder>;
        public:
            InstructionDecoder()  = default;
            virtual ~InstructionDecoder() = default;
            static InstructionDecoder::Ref Create(uint64_t memoryOffset);

            bool Decode(CPUBase &cpu);
            // Converts a decoded instruction back to it's mnemonic form
            std::string ToString() const;

            // const RegisterValue &GetDstValue() {
            //     return dstValue;
            // }
            // const RegisterValue &GetSrcValue() {
            //     return srcValue;
            // }
            const RegisterValue &GetValue() {
                return value;
            }

            size_t GetInstrSizeInBytes() {
                return ofsEndInstr - ofsStartInstr;
            }
            uint64_t GetInstrStartOfs() {
                return ofsStartInstr;
            }
            uint64_t GetInstrEndOfs() {
                return ofsEndInstr;
            }

        private:
            uint8_t NextByte(CPUBase &cpu);
            // Helper for 'ToString'
            std::string DisasmOperand(AddressMode addrMode, uint8_t regIndex) const;
            // Perhaps move to base class
            RegisterValue ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, int idxRegister);



        public:
            // Used during by decoder...
            uint8_t opCode;
            OperandClass opClass;
            OperandDescription description;

            OperandSize szOperand; // Only if 'description.features & OperandSize' == true
            uint8_t dstRegAndFlags; // Always set
            uint8_t srcRegAndFlags; // Only if 'description.features & TwoOperands' == true

            AddressMode dstAddrMode; // Always decoded from 'dstRegAndFlags'
            uint8_t dstRegIndex; // decoded like: (dstRegAndFlags>>4) & 15;
//            RegisterValue dstValue; // this can be an immediate or something else, essentially result from operand


            // Only if 'description.features & TwoOperands' == true
            AddressMode srcAddrMode; // decoded from srcRegAndFlags
            uint8_t srcRegIndex; // decoded like: (srcRegAndFlags>>4) & 15;
//            RegisterValue srcValue; // this can be an immediate or register, essentially result from operand

            // There can only be ONE immediate value associated with an instruction
            RegisterValue value; // this can be an immediate or something else, essentially result from operand

        private:
            uint64_t memoryOffset = {};
            uint64_t ofsStartInstr = {};
            uint64_t ofsEndInstr = {};
        };
    }
}



#endif //INSTRUCTIONDECODER_H
