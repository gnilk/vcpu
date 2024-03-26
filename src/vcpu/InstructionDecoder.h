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

            RegisterValue ReadSrcValue(CPUBase &cpu);
            RegisterValue ReadDstValue(CPUBase &cpu);

            uint64_t ComputeRelativeAddress(CPUBase &cpuBase, const RelativeAddressing &relAddr);

            struct Operand {
                // Used during by decoder...
                uint8_t opCodeByte;     // raw opCodeByte
                OperandCode opCode;
                OperandDescription description;

                uint8_t opSizeAndFamilyCode;    // raw 'OperandSize' byte - IF instruction feature declares this is valid
                OperandSize opSize; // Only if 'description.features & OperandSize' == true
                OperandFamily opFamily;
            };
            struct OperandArg {
                uint8_t regAndFlags = 0;
                uint8_t regIndex = 0;
                AddressMode addrMode = AddressMode::Absolute;
                uint64_t absoluteAddr = 0;
                RelativeAddressing relAddrMode = {};
            };

        protected:
            uint8_t NextByte(CPUBase &cpu);
            // Helper for 'ToString'
            std::string DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionDecoder::RelativeAddressing relAddr) const;
            // Perhaps move to base class
            RegisterValue ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, uint64_t absAddress, RelativeAddressing relAddr, int idxRegister);

            void DecodeOperandArg(CPUBase &cpu, OperandArg &inOutOpArg);
            void DecodeOperandArgAddrMode(CPUBase &cpu, OperandArg &inOutOpArg);
        public:
            // Used during by decoder...

            Operand code;
            OperandArg opArgDst;
            OperandArg opArgSrc;

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

            size_t GetInstrSizeInBytes() const {
                return instrDecoder->GetInstrSizeInBytes();
            }
            uint64_t GetInstrStartOfs() const {
                return instrDecoder->GetInstrStartOfs();
            }
            uint64_t GetInstrEndOfs() const {
                return instrDecoder->GetInstrEndOfs();
            }


        };

    }
}



#endif //INSTRUCTIONDECODER_H
