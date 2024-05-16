//
// Created by gnilk on 16.05.24.
//

#include <memory>
#include <utility>
#include "SIMDInstructionDecoder.h"

using namespace gnilk;
using namespace gnilk::vcpu;


InstructionDecoderBase::Ref SIMDInstructionDecoder::Create(uint64_t memoryOffset) {

    auto inst = std::make_shared<SIMDInstructionDecoder>();
    inst->memoryOffset = memoryOffset;
    inst->ofsStartInstr = 0;
    inst->ofsEndInstr = 0;
    return inst;
}


bool SIMDInstructionDecoder::Tick(CPUBase &cpu) {
    switch(state) {
        case State::kStateIdle :
            return ExecuteTickFromIdle(cpu);
    }
    return false;
}
bool SIMDInstructionDecoder::ExecuteTickFromIdle(CPUBase &cpu) {


    memoryOffset = cpu.GetInstrPtr().data.longword;
    ofsStartInstr = memoryOffset;
    ofsEndInstr = memoryOffset;

    auto opCodeByte = NextByte(cpu);
    if (!SIMDInstructionSet::GetInstructionSet().contains(static_cast<SimdOpCode>(opCodeByte))) {
        // We failed..
        cpu.AdvanceInstrPtr(1);
        return false;
    }



    ChangeState(State::kStateIdle);
    return false;
}

