//
// Created by gnilk on 26.05.24.
//

#include "SIMDInstructionSet.h"

#include "SIMDInstructionSetDef.h"
#include "SIMDInstructionSetImpl.h"
#include "SIMDInstructionDecoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;

InstructionSet gnilk::vcpu::glb_InstructionSetSIMD = {
        .extByte = 0,
        .decoder = SIMDInstructionDecoder(),
        .definition = SIMDInstructionSetDef(),
        .implementation = SIMDInstructionSetImpl(),

};
