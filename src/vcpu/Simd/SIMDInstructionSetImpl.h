//
// Created by gnilk on 24.05.24.
//

#ifndef VCPU_SIMDINSTRUCTIONSETIMPL_H
#define VCPU_SIMDINSTRUCTIONSETIMPL_H

#include "InstructionSetV1/InstructionSetV1Impl.h"
namespace gnilk {
    namespace vcpu {

        class SIMDInstructionDecoder;

        class SIMDInstructionSetImpl : public InstructionSetImplBase {
        public:
            bool ExecuteInstruction(CPUBase &cpu, InstructionDecoderBase &decoder) override;
        protected:
            void ExecuteLoad(CPUBase &cpu, SIMDInstructionDecoder &decoder);
        };
    }
}


#endif //VCPU_SIMDINSTRUCTIONSETIMPL_H
