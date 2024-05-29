//
// Created by gnilk on 24.05.24.
//

#include "SIMDInstructionSetImpl.h"
#include "SIMDInstructionDecoder.h"
#include "SIMDInstructionSetDef.h"

using namespace gnilk;
using namespace gnilk::vcpu;
bool SIMDInstructionSetImpl::ExecuteInstruction(CPUBase &cpu) {

    SIMDInstructionSetDef::Operand decoderOutput;
    if (!cpu.GetDispatch().Pop(&decoderOutput, sizeof(decoderOutput))) {
        fmt::println(stderr, "[SIMDInstructionSetImpl::ExecuteInstruction] Unable to fetch decoder output from dispatcher!");
        return false;
    }

    switch(decoderOutput.opCode) {
        case SimdOpCode::LOAD :
            ExecuteLoad(cpu, decoderOutput);
            break;
        default:
            return false;
    }
    // Now we can proceed...
    return true;
}

void SIMDInstructionSetImpl::ExecuteLoad(CPUBase &cpu, SIMDInstructionSetDef::Operand &operand) {
    printf("Execute Load\n");
}