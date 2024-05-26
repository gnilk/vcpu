//
// Created by gnilk on 16.05.24.
//

#ifndef VCPU_SIMDINSTRUCTIONDECODER_H
#define VCPU_SIMDINSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "SIMDInstructionSetDef.h"
#include "SIMDInstructionSetImpl.h"
#include "InstructionDecoderBase.h"
#include "CPUBase.h"


namespace gnilk {
    namespace vcpu {
        class SIMDInstructionDecoder : public InstructionDecoderBase {
            friend SIMDInstructionSetImpl;
        public:
            SIMDInstructionDecoder() = default;
            virtual ~SIMDInstructionDecoder() = default;


            static SIMDInstructionDecoder::Ref Create();

            bool Decode(CPUBase &cpu);
            bool Tick(CPUBase &cpu);
        protected:
            bool IsComplete() override;

            bool ExecuteTickFromIdle(CPUBase &cpu);
            bool ExecuteTickReadWriteMem(CPUBase &cpu);

            void ReadMem(CPUBase &cpu);
            void WriteMem(CPUBase &cpu);
            size_t ComputeInstrSize() const;
        protected:
            enum class State : uint8_t {
                kStateIdle,
                kStateReadWriteMem,
                kStateFinished,
            };
            State state = {};

            const State &GetState() {
                return state;
            }
            void ChangeState(State newState) {
                state = newState;
            }
        protected:
            struct Operand {
                uint8_t opCodeByte;
                SimdOpCode opCode;
                OperandDescriptionBase description;

                uint8_t opFlagsHighByte;
                kSimdOpSize opSize;
                kSimdAddrMode opAddrMode;

                uint8_t opFlagsLowBitsAndDst;
                uint8_t opFlagsLowBits;
                uint8_t opDstRegIndex;

                uint8_t opSrcAAndMaskOrSrcB;
                uint8_t opSrcAIndex;
                union {
                    uint8_t opSrcBIndex;
                    uint8_t opMask;
                };

            };

            Operand operand = {};

        };
    }
}


#endif //VCPU_SIMDINSTRUCTIONDECODER_H
