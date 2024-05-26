//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSETV1_H
#define VCPU_INSTRUCTIONSETV1_H

#include "InstructionSet.h"
#include "InstructionSetV1Decoder.h"
#include "InstructionSetV1Def.h"
#include "InstructionSetV1Impl.h"


namespace gnilk {
    namespace vcpu {

        class InstructionSetV1 : public
                InstructionSetInst<InstructionSetV1Decoder, InstructionSetV1Def, InstructionSetV1Impl> {
        public:
            InstructionDecoderBase::Ref CreateDecoder() override;
        };

        //extern InstructionSetV1 glb_InstructionSetV1;
    }
}

#endif //VCPU_INSTRUCTIONSETV1_H
