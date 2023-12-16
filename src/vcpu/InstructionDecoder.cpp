//
// Created by gnilk on 16.12.23.
//

#include "InstructionDecoder.h"
#include <utility>
#include <fmt/core.h>

using namespace gnilk;
using namespace gnilk::vcpu;

InstructionDecoder::Ref InstructionDecoder::Create(OperandClass opClass) {
    // check if we have this instruction defined
    if (!gnilk::vcpu::GetInstructionSet().contains(opClass)) {
        return nullptr;
    }

    auto inst = std::make_shared<InstructionDecoder>();

    inst->opClass = opClass;
    inst->description = gnilk::vcpu::GetInstructionSet().at(opClass);

    return inst;
}

bool InstructionDecoder::Decode(CPUBase &cpu) {
    //
    // Setup addressing
    //
    uint8_t szAddressing = 0;   // FIXME: default!!!

    if (description.features & OperandDescriptionFlags::OperandSize) {
        szAddressing = cpu.FetchByteFromInstrPtr();
        szOperand = static_cast<OperandSize>(szAddressing);
    }

    //
    // Setup src/dst handling
    //
    dstRegAndFlags = 0;
    srcRegAndFlags = 0;

    if (description.features & OperandDescriptionFlags::TwoOperands) {
        dstRegAndFlags = cpu.FetchByteFromInstrPtr();
        srcRegAndFlags = cpu.FetchByteFromInstrPtr();
    } else {
        dstRegAndFlags = cpu.FetchByteFromInstrPtr();
    }

    // This bit-fiddeling we can do anyway...
    dstAddrMode = static_cast<AddressMode>(dstRegAndFlags & 0x0f);
    srcAddrMode = static_cast<AddressMode>(srcRegAndFlags & 0x0f);

    dstReg = (dstRegAndFlags>>4) & 15;
    srcReg = (srcRegAndFlags>>4) & 15;

    return true;
}

