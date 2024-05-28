//
// Created by gnilk on 25.05.24.
//

#include "InstructionSetV1.h"

#include "InstructionSetV1Def.h"
#include "InstructionSetV1Impl.h"
#include "InstructionSetV1Decoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;



// We don't use this here - it is simply 0 - as we are the root, can easily modify later if needed...
InstructionDecoderBase::Ref InstructionSetV1::CreateDecoder(uint8_t instrTypeId) {
    return std::make_shared<InstructionSetV1Decoder>();
}