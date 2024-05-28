//
// Created by gnilk on 16.12.23.
//

#ifndef INSTRUCTIONDECODER_H
#define INSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "CPUBase.h"
#include "InstructionDecoderBase.h"

#include "InstructionSetV1Def.h"

namespace gnilk {
    namespace vcpu {

        class InstructionSetV1Impl;
        class InstructionSetV1Decoder : public InstructionDecoderBase {
            friend InstructionSetV1Impl;
        public:
            using Ref = std::shared_ptr<InstructionSetV1Decoder>;

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
            InstructionSetV1Decoder()  {
                printf("InstructionSetV1Decoder::CTOR, this=%p\n", (void *)this);
            }
            virtual ~InstructionSetV1Decoder() = default;
            static InstructionSetV1Decoder::Ref Create();

            void Reset() override;
            bool Tick(CPUBase &cpu) override;

            // Make this private when it works
            bool ExecuteTickFromIdle(CPUBase &cpu);
            bool ExecuteTickDecodeAddrMode(CPUBase &cpu);
            bool ExecuteTickReadMem(CPUBase &cpu);
            bool ExecuteTickDecodeExt(CPUBase &cpu);


            bool IsIdle() override { return (state == State::kStateIdle); }
            bool IsFinished() override { return (state == State::kStateFinished); }


            // Converts a decoded instruction back to it's mnemonic form
            std::string ToString() const override;

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
                OperandDescriptionBase description;

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
            std::string DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionSetV1Decoder::RelativeAddressing relAddr) const;
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

            InstructionDecoderBase::Ref GetDecoderForExtension(uint8_t ext);
        public:
            // FIXME: This is the result of the decoding
            //        Move this to a very specific place so we can 'queue' it up - this makes the decoder separate from the instr.impl..
            Operand code;
            OperandArg opArgDst;
            OperandArg opArgSrc;

            // There can only be ONE immediate value associated with an instruction
            RegisterValue value; // this can be an immediate or something else, essentially result from operand
            // End of result


            std::unordered_map<uint8_t, InstructionDecoderBase::Ref> extDecoders = {};
        };

        // This is more or less a wrapper around the InstructionDecoder
        // Because we need to hold more information
        struct LastInstruction {
            Registers cpuRegistersBefore;
            Registers cpuRegistersAfter;
            CPUISRState isrStateBefore;
            CPUISRState isrStateAfter;
            InstructionSetV1Decoder instrDecoder = {};       // FIXME: Try to make this 'InstructionDecoderBase::Ref'

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
