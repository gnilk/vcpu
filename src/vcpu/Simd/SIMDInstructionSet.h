//
// Created by gnilk on 26.05.24.
//

#ifndef VCPU_SIMDINSTRUCTIONSET_H
#define VCPU_SIMDINSTRUCTIONSET_H

#include "InstructionSet.h"

#include "SIMDInstructionSetDef.h"
#include "SIMDInstructionSetImpl.h"
#include "SIMDInstructionDecoder.h"

namespace gnilk {
    namespace vcpu {
        class InstructionSetSIMD : public
                InstructionSetInst<SIMDInstructionDecoder, SIMDInstructionSetDef, SIMDInstructionSetImpl> {
        public:
            InstructionDecoderBase::Ref CreateDecoder() override;
        };

        extern InstructionSetSIMD glb_InstructionSetSIMD;
    }
}


#endif //VCPU_SIMDINSTRUCTIONSET_H
