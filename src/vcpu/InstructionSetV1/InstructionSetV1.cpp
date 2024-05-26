//
// Created by gnilk on 25.05.24.
//

#include "InstructionSetV1.h"


// FIXME: Align these filenames
#include "InstructionSetDefV1.h"
#include "InstructionSetV1Impl.h"
#include "InstructionDecoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;

InstructionSet gnilk::vcpu::glb_InstructionSetV1 = {
        .extByte = 0,
        .decoder = InstructionDecoder(),
        .definition = InstructionSetV1Def(),
        .implementation = InstructionSetV1Impl(),

};
