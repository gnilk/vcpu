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

        // See 'InstructionDecoderBase' for details
        class SIMDInstructionDecoder : public InstructionDecoderBase {
            friend SIMDInstructionSetImpl;
        public:
            SIMDInstructionDecoder() = default;

            // Create the decoder but use this as the instruction type id when pushing to the dispatcher...
            SIMDInstructionDecoder(uint8_t useInstrTypeId) : instrTypeId(useInstrTypeId) {

            }
            virtual ~SIMDInstructionDecoder() = default;


            static SIMDInstructionDecoder::Ref Create();

            bool Tick(CPUBase &cpu) override;

            bool IsFinished() override;
            bool IsIdle() override;
            void Reset() override;

            bool Finalize(CPUBase &cpu) override;
            bool PushToDispatch(CPUBase &cpu);


        protected:

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
            uint8_t instrTypeId = 0;        // 0 is for root instructions, i.e. when running on it's own..
            SIMDInstructionSetDef::Operand operand = {};

        };
    }
}


#endif //VCPU_SIMDINSTRUCTIONDECODER_H
