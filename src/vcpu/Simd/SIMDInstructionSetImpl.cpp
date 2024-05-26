//
// Created by gnilk on 24.05.24.
//

#include "SIMDInstructionSetImpl.h"
#include "SIMDInstructionDecoder.h"
#include "SIMDInstructionSetDef.h"

using namespace gnilk;
using namespace gnilk::vcpu;
bool SIMDInstructionSetImpl::ExecuteInstruction(CPUBase &cpu, InstructionDecoderBase &decoder) {
    auto &baseDecoder = decoder.GetCurrentExtDecoder();
    auto &simdDecoder = dynamic_cast<SIMDInstructionDecoder &>(baseDecoder);

    switch(simdDecoder.operand.opCode) {
        case SimdOpCode::LOAD :
            ExecuteLoad(cpu, simdDecoder);
            break;
        default:
            return false;
    }
    // Now we can proceed...
    return true;
}

void SIMDInstructionSetImpl::ExecuteLoad(CPUBase &cpu, SIMDInstructionDecoder &decoder) {
    printf("Execute Load\n");
}