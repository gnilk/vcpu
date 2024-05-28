//
// Created by gnilk on 16.12.23.
//

#ifndef INSTRUCTIONDECODER_H
#define INSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "CPUBase.h"
#include "InstructionDecoderBase.h"
#include "Dispatch.h"

#include "InstructionSetV1Def.h"

namespace gnilk {
    namespace vcpu {

        // FIXME: Could do with some cleanup...
        class InstructionSetV1Decoder : public InstructionDecoderBase {
        public:
            using Ref = std::shared_ptr<InstructionSetV1Decoder>;

            enum class State : uint8_t {
                kStateIdle,
                kStateDecodeAddrMode,
                kStateReadMem,
                kStateTwoOpDstReadMem,
                kStateFinished,
                kStateDecodeExtension,      // Decoding is deferred to extension..
            };
            State state = {};
        public:
            static const std::string &StateToString(State s);

        public:
            InstructionSetV1Decoder()  = default;
            virtual ~InstructionSetV1Decoder() = default;
            static InstructionSetV1Decoder::Ref Create();

            void Reset() override;
            bool Tick(CPUBase &cpu) override;
            bool Finalize(CPUBase &cpu);
            bool PushToDispatch(CPUBase &cpu);


            // Make this private when it works
            bool ExecuteTickFromIdle(CPUBase &cpu);
            bool ExecuteTickDecodeAddrMode(CPUBase &cpu);
            bool ExecuteTickReadMem(CPUBase &cpu);
            bool ExecuteTickReadDstMem(CPUBase &cpu);
            bool ExecuteTickDecodeExt(CPUBase &cpu);


            bool IsIdle() override { return (state == State::kStateIdle); }
            bool IsFinished() override { return (state == State::kStateFinished); }


            // Converts a decoded instruction back to it's mnemonic form
            std::string ToString() const override;

            RegisterValue &GetPrimaryValue() {
                return primaryValue;
            }
            RegisterValue &GetSecondaryValue() {
                return secondaryValue;
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


        protected:
            // Helper for 'ToString'
            std::string DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionSetV1Def::RelativeAddressing relAddr) const;
            // Perhaps move to base class
            RegisterValue ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, uint64_t absAddress, InstructionSetV1Def::RelativeAddressing relAddr, int idxRegister);

            void DecodeOperandArg(CPUBase &cpu, InstructionSetV1Def::DecodedOperandArg &inOutOpArg);
            void DecodeOperandArgAddrMode(CPUBase &cpu, InstructionSetV1Def::DecodedOperandArg &inOutOpArg);

            size_t ComputeInstrSize() const;
            size_t ComputeOpArgSize(const InstructionSetV1Def::DecodedOperandArg &opArg) const;
            uint64_t ComputeRelativeAddress(CPUBase &cpuBase, const InstructionSetV1Def::RelativeAddressing &relAddr) const;
            bool IsExtension(uint8_t opCodeByte) const;

            const State &GetState() {
                return state;
            }
            void ChangeState(State newState);

            InstructionDecoderBase::Ref GetDecoderForExtension(uint8_t ext);

        public:
            // This is moved into 'InstructionSetV1Def::DecoderOperand' structure during PushToDispatch
            // FIXME: Could be moved to 'DecoderOperand' directly - public because of unit-testing..
            InstructionSetV1Def::DecodedOperand code;
            InstructionSetV1Def::DecodedOperandArg opArgDst;
            InstructionSetV1Def::DecodedOperandArg opArgSrc;

            // This is the primary value - result of the first memory read, used by most instructions.
            // Note: This can be either the destination or source
            RegisterValue primaryValue; // this can be an immediate or something else, essentially result from operand

            // This is the secondary value - result of second memory - if required by instruction (see InstructionSetV1Def)
            // Note: This is _ALWAYS_ the destination
            RegisterValue secondaryValue;

            InstructionDecoderBase::Ref currentExtDecoder = nullptr;
        };

        // This is more or less a wrapper around the InstructionDecoder
        // Because we need to hold more information
        // FIXME: Reconsider how this should work...
        struct LastInstruction {
            Registers cpuRegistersBefore;
            Registers cpuRegistersAfter;
            CPUISRState isrStateBefore;
            CPUISRState isrStateAfter;

            // This actually holds the instruction decoder instance - this is not needed - we should be able to use any
            InstructionSetV1Decoder instrDecoder = {};       // FIXME: Try to make this 'InstructionDecoderBase::Ref'

            CPUISRState GetISRStateBefore() const {
                return isrStateBefore;
            }

            std::string ToString() const {
                return instrDecoder.ToString();     // FIXME: Not sure this belongs here (after refactoring) -> move to InstructionSetDef
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
