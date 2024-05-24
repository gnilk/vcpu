//
// Created by gnilk on 24.05.24.
//

#include "SIMDInstructionSetImpl.h"
#include "SIMDInstructionDecoder.h"
#include "SIMDInstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;
bool SIMDInstructionSetImpl::ExecuteInstruction(InstructionDecoder &decoder) {
    auto baseDecoder = decoder.GetCurrentExtDecoder();
    auto simdDecoder = std::dynamic_pointer_cast<SIMDInstructionDecoder>(baseDecoder);

    switch(simdDecoder->operand.opCode) {
        case SimdOpCode::LOAD :
            ExecuteLoad(*simdDecoder);
            break;
        default:
            return false;
    }
    // Now we can proceed...
    return true;
}

void SIMDInstructionSetImpl::ExecuteLoad(SIMDInstructionDecoder &decoder) {
    printf("Execute Load\n");
}