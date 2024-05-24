//
// Created by gnilk on 24.05.24.
//

#ifndef VCPU_SIMDINSTRUCTIONSETIMPL_H
#define VCPU_SIMDINSTRUCTIONSETIMPL_H

#include "InstructionSetImpl.h"
namespace gnilk {
    namespace vcpu {

        class SIMDInstructionDecoder;

        class SIMDInstructionSetImpl : public InstructionSetImplBase {
        public:
            bool ExecuteInstruction(InstructionDecoder &decoder) override;
        protected:
            void ExecuteLoad(SIMDInstructionDecoder &decoder);
        };
    }
}


#endif //VCPU_SIMDINSTRUCTIONSETIMPL_H
