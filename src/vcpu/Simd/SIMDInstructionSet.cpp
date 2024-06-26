//
// Created by gnilk on 26.05.24.
//

#include "SIMDInstructionSet.h"
#include "SIMDInstructionDecoder.h"

using namespace gnilk;
using namespace gnilk::vcpu;


InstructionSetSIMD gnilk::vcpu::glb_InstructionSetSIMD;

InstructionDecoderBase::Ref InstructionSetSIMD::CreateDecoder(uint8_t instrTypeId) {
    return std::make_shared<SIMDInstructionDecoder>(instrTypeId);
}
