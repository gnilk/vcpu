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
            struct RelativeAddressing {
                RelativeAddressMode mode;
                // not sure this is a good idea
                union {
                    uint8_t absoulte;
                    struct {
                        uint8_t shift : 4;      // LSB
                        uint8_t index : 4;      // MSB
                    } reg;
                } relativeAddress;
            };
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
            std::string DisasmOperand(AddressMode addrMode, uint8_t regIndex, InstructionDecoder::RelativeAddressing relAddr) const;
            // Perhaps move to base class
            RegisterValue ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, RelativeAddressing relAddr, int idxRegister);

            void DecodeDstReg(CPUBase &cpu);
            void DecodeSrcReg(CPUBase &cpu);

        public:

            // Used during by decoder...
            uint8_t opCodeByte;     // raw opCodeByte
            OperandCode opCode;
            OperandDescription description;


            uint8_t opSizeAndFamilyCode;    // raw 'OperandSize' byte - IF instruction feature declares this is valid
            OperandSize opSize; // Only if 'description.features & OperandSize' == true
            OperandFamily opFamily;

            uint8_t dstRegAndFlags; // Always set
            uint8_t srcRegAndFlags; // Only if 'description.features & TwoOperands' == true

            AddressMode dstAddrMode;        // decoded from 'dstRegAndFlags'
            RelativeAddressing dstRelAddrMode;  // decoded from 'dstRegAndFlags'
            uint8_t dstRegIndex; // decoded like: (dstRegAndFlags>>4) & 15;

            // Only if 'description.features & TwoOperands' == true
            AddressMode srcAddrMode;        // decoded from srcRegAndFlags
            RelativeAddressing srcRelAddrMode;  // decoded from srcRegAndFlags


            uint8_t srcRegIndex; // decoded like: (srcRegAndFlags>>4) & 15;

            // There can only be ONE immediate value associated with an instruction
            RegisterValue value; // this can be an immediate or something else, essentially result from operand

        private:
            uint64_t memoryOffset = {};
            uint64_t ofsStartInstr = {};
            uint64_t ofsEndInstr = {};
        };

        // This is more or less a wrapper around the InstructionDecoder
        // Because we need to hold more information
        struct LastInstruction {
            Registers cpuRegistersBefore;
            Registers cpuRegistersAfter;
            CPUISRState isrStateBefore;
            CPUISRState isrStateAfter;
            InstructionDecoder::Ref instrDecoder;

            CPUISRState GetISRStateBefore() const {
                return isrStateBefore;
            }

            std::string ToString() const {
                if (instrDecoder == nullptr) {
                    return "";
                }
                return instrDecoder->ToString();
            }
        };

    }
}



#endif //INSTRUCTIONDECODER_H
