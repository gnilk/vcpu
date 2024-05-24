//
// Created by gnilk on 16.12.23.
//

#ifndef INSTRUCTIONDECODER_H
#define INSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "InstructionDecoderBase.h"
#include "InstructionSet.h"
#include "CPUBase.h"

namespace gnilk {
    namespace vcpu {

        // FIXME: MOVE THIS - impl is in 'InstructionSet'
        InstructionDecoderBase::Ref GetDecoderForExtension(uint8_t extCode);


        class InstructionDecoder : public InstructionDecoderBase {
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
            enum class State : uint8_t {
                kStateIdle,
                kStateDecodeAddrMode,
                kStateReadMem,
                kStateFinished,
                kStateDecodeExtension,      // Decoding is deferred to extension..
            };
            State state = {};
        public:
            static const std::string &StateToString(State s);

        public:
            InstructionDecoder()  = default;
            virtual ~InstructionDecoder() = default;
            static InstructionDecoder::Ref Create(uint64_t memoryOffset);

            void Reset() override;
            bool Decode(CPUBase &cpu);          // Single pass decoding - executes all ticks
            bool Tick(CPUBase &cpu) override;
            // Make this private when it works
            bool ExecuteTickFromIdle(CPUBase &cpu);
            bool ExecuteTickDecodeAddrMode(CPUBase &cpu);
            bool ExecuteTickReadMem(CPUBase &cpu);
            bool ExecuteTickDecodeExt(CPUBase &cpu);
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

            // Remove these
            size_t GetInstrSizeInBytes() {
                return ComputeInstrSize();
            }
            size_t GetInstrSizeInBytes() const {
                return ComputeInstrSize();
            }
            uint64_t GetInstrStartOfs() {
                return ofsStartInstr;
            }
            uint64_t GetInstrStartOfs() const {
                return ofsStartInstr;
            }
            uint64_t GetInstrEndOfs() {
                return ofsEndInstr;
            }
            uint64_t GetInstrEndOfs() const {
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
                AddressMode addrMode = {}; //AddressMode::Absolute;
                uint64_t absoluteAddr = 0;
                RelativeAddressing relAddrMode = {};
            };

        protected:
            // Helper for 'ToString'
            std::string DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionDecoder::RelativeAddressing relAddr) const;
            // Perhaps move to base class
            RegisterValue ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, uint64_t absAddress, RelativeAddressing relAddr, int idxRegister);

            void DecodeOperandArg(CPUBase &cpu, OperandArg &inOutOpArg);
            void DecodeOperandArgAddrMode(CPUBase &cpu, OperandArg &inOutOpArg);

            size_t ComputeInstrSize() const;
            size_t ComputeOpArgSize(const OperandArg &opArg) const;
            bool IsExtension(uint8_t opCodeByte) const;

            const State &GetState() {
                return state;
            }
            void ChangeState(State newState) {
                state = newState;
            }

            InstructionDecoderBase::Ref GetDecoderForInstrExt(uint8_t ext);

            InstructionDecoderBase::Ref extDecoder = nullptr;

        public:
            // FIXME: TEMP TEMP
            InstructionDecoderBase::Ref GetCurrentExtDecoder() {
                return extDecoder;
            }


            // Used during by decoder...

            Operand code;
            OperandArg opArgDst;
            OperandArg opArgSrc;

            // There can only be ONE immediate value associated with an instruction
            RegisterValue value; // this can be an immediate or something else, essentially result from operand

        };

        // This is more or less a wrapper around the InstructionDecoder
        // Because we need to hold more information
        struct LastInstruction {
            Registers cpuRegistersBefore;
            Registers cpuRegistersAfter;
            CPUISRState isrStateBefore;
            CPUISRState isrStateAfter;
            InstructionDecoder instrDecoder;

            CPUISRState GetISRStateBefore() const {
                return isrStateBefore;
            }

            std::string ToString() const {
                return instrDecoder.ToString();
            }

            size_t GetInstrSizeInBytes() const {
                return instrDecoder.GetInstrSizeInBytes();
            }
            uint64_t GetInstrStartOfs() const {
                return instrDecoder.GetInstrStartOfs();
            }
            uint64_t GetInstrEndOfs() const {
                return instrDecoder.GetInstrEndOfs();
            }


        };

    }
}



#endif //INSTRUCTIONDECODER_H
