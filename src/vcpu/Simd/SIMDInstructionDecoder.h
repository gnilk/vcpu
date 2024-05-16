//
// Created by gnilk on 16.05.24.
//

#ifndef VCPU_SIMDINSTRUCTIONDECODER_H
#define VCPU_SIMDINSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "SIMDInstructionSet.h"
#include "InstructionDecoderBase.h"
#include "CPUBase.h"


namespace gnilk {
    namespace vcpu {
        class SIMDInstructionDecoder : public InstructionDecoderBase {
        public:
            SIMDInstructionDecoder() = default;
            virtual ~SIMDInstructionDecoder() = default;


            static SIMDInstructionDecoder::Ref Create(uint64_t memoryOffset);

            bool Tick(CPUBase &cpu);
        protected:
            bool ExecuteTickFromIdle(CPUBase &cpu);

        protected:
            enum class State : uint8_t {
                kStateIdle,
                kStateDecodeAddrMode,
                kStateReadMem,
                kStateFinished,
            };
            State state = {};

            const State &GetState() {
                return state;
            }
            void ChangeState(State newState) {
                state = newState;
            }

        };
    }
}


#endif //VCPU_SIMDINSTRUCTIONDECODER_H
