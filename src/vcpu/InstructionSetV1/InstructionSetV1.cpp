//
// Created by gnilk on 25.05.24.
//

#include "InstructionSetV1.h"

#include "InstructionSetV1Def.h"
#include "InstructionSetV1Impl.h"
#include "InstructionSetV1Decoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;

InstructionSet gnilk::vcpu::glb_InstructionSetV1 = {
        .extByte = 0,
        .decoder = InstructionSetV1Decoder(),
        .definition = InstructionSetV1Def(),
        .implementation = InstructionSetV1Impl(),

};
