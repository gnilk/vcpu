//
// Created by gnilk on 25.05.24.
//

#include "InstructionSetV1.h"

#include "InstructionSetV1Def.h"
#include "InstructionSetV1Impl.h"
#include "InstructionSetV1Decoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;



InstructionDecoderBase::Ref InstructionSetV1::CreateDecoder() {
    return std::make_shared<InstructionSetV1Decoder>();
}