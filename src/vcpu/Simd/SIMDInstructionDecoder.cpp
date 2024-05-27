//
// Created by gnilk on 16.05.24.
//

#include <memory>
#include <utility>
#include <assert.h>

#include "SIMDInstructionSet.h"
#include "SIMDInstructionDecoder.h"

using namespace gnilk;
using namespace gnilk::vcpu;


InstructionDecoderBase::Ref SIMDInstructionDecoder::Create() {

    auto inst = std::make_shared<SIMDInstructionDecoder>();
    return inst;
}

bool SIMDInstructionDecoder::IsFinished() {
    return (state == State::kStateFinished);
}
bool SIMDInstructionDecoder::IsIdle() {
    return (state == State::kStateIdle);
}
void SIMDInstructionDecoder::Reset() {
    ChangeState(State::kStateIdle);
}

bool SIMDInstructionDecoder::Tick(CPUBase &cpu) {
    switch(state) {
        case State::kStateIdle :
            return ExecuteTickFromIdle(cpu);
        case State::kStateReadWriteMem :
            return ExecuteTickReadWriteMem(cpu);
        case State::kStateFinished :
            return true;
    }
    return false;
}

bool SIMDInstructionDecoder::ExecuteTickFromIdle(CPUBase &cpu) {

    memoryOffset = cpu.GetInstrPtr().data.longword;
    ofsStartInstr = memoryOffset;
    ofsEndInstr = memoryOffset;

    auto &instructionSet = glb_InstructionSetSIMD.GetDefinition();

    auto opCodeByte = NextByte(cpu);
    if (!instructionSet.GetInstructionSet().contains(static_cast<SimdOpCode>(opCodeByte))) {
        // We failed..
        cpu.AdvanceInstrPtr(1);
        return false;
    }

    // this is valid - so begin
    operand.opCodeByte = opCodeByte;
    operand.opCode = static_cast<SimdOpCode>(opCodeByte);
    operand.description = instructionSet.GetInstructionSet().at(operand.opCode);

    operand.opFlagsHighByte = NextByte(cpu);
    operand.opSize = static_cast<kSimdOpSize>(operand.opFlagsHighByte &  kSimdFlagOpSizeBitMask);
    operand.opAddrMode = static_cast<kSimdAddrMode>(operand.opFlagsHighByte & kSimdFlagAddrRegBitMask);

    operand.opFlagsLowBitsAndDst = NextByte(cpu);
    operand.opFlagsLowBits = operand.opFlagsLowBitsAndDst & 0xf0;
    // Decode destination register index
    operand.opDstRegIndex = operand.opFlagsLowBitsAndDst & 0x0f;

    // Decode src A index
    operand.opSrcAAndMaskOrSrcB = NextByte(cpu);
    operand.opSrcAIndex = operand.opSrcAAndMaskOrSrcB & 0xf0;

    // One is enough - it's a union...
    operand.opSrcBIndex = operand.opSrcAAndMaskOrSrcB & 0x0f;
    operand.opMask = operand.opSrcAAndMaskOrSrcB & 0x0f;

    ofsEndInstr = memoryOffset;
    auto szComputed = ComputeInstrSize();

    // fixed size instr.
    assert(szComputed == 4);



    // now we know how much data to consume for this instruction - advance to next - which allows us to proceed next tick
    // and the Pipeline to grab a new instruction..
    cpu.AdvanceInstrPtr(szComputed);

    // IF we have an address register - we should do a read/write mem operation
    if (operand.opAddrMode != kSimdAddrMode::kOpAddrMode_Simd) {

        // Execute directly?
        ChangeState(State::kStateFinished);
    } else {
        ChangeState(State::kStateReadWriteMem);
    }

    return true;
}

bool SIMDInstructionDecoder::ExecuteTickReadWriteMem(CPUBase &cpu) {
    if (operand.opAddrMode == kSimdAddrMode::kOpAddrMode_SrcReg) {
        ReadMem(cpu);
    }

    // Execute

    if (operand.opAddrMode == kSimdAddrMode::kOpAddrMode_DstReg) {
        WriteMem(cpu);
    }
    ChangeState(State::kStateFinished);
    return true;
}

void SIMDInstructionDecoder::ReadMem(CPUBase &cpu) {

}
void SIMDInstructionDecoder::WriteMem(CPUBase &cpu) {

}



// SIMD instructions are fixed size...
size_t SIMDInstructionDecoder::ComputeInstrSize() const {
    size_t opSize = ofsEndInstr - ofsStartInstr;    // Start here - as operands have different sizes..
    return opSize;
}

