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

    // FIXME: refactor dst/src value -> value, we do NOT write values back to the CPU but we do READ data as part of decoding

    if (description.features & OperandDescriptionFlags::OneOperand) {
        dstRegAndFlags = cpu.FetchByteFromInstrPtr();

        dstAddrMode = static_cast<AddressMode>(dstRegAndFlags & 0x0f);
        dstRegIndex = (dstRegAndFlags>>4) & 15;

        value = ReadFrom(cpu, szOperand, dstAddrMode, dstRegIndex);
    } else if (description.features & OperandDescriptionFlags::TwoOperands) {
        dstRegAndFlags = cpu.FetchByteFromInstrPtr();
        srcRegAndFlags = cpu.FetchByteFromInstrPtr();

        dstAddrMode = static_cast<AddressMode>(dstRegAndFlags & 0x0f);
        srcAddrMode = static_cast<AddressMode>(srcRegAndFlags & 0x0f);

        dstRegIndex = (dstRegAndFlags>>4) & 15;
        srcRegIndex = (srcRegAndFlags>>4) & 15;

        value = ReadFrom(cpu, szOperand, srcAddrMode, srcRegIndex);
//        dstValue = ReadFrom(cpu, szOperand, dstAddrMode, dstRegIndex);

    } else {
        // No operands
    }


    return true;
}

RegisterValue InstructionDecoder::ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, int idxRegister) {
    RegisterValue v = {};

    // This should be performed by instr. decoder...
    if (addrMode == AddressMode::Immediate) {
        return cpuBase.ReadImmediateMode(szOperand);
    } else if (addrMode == AddressMode::Register) {
        auto &reg = cpuBase.GetRegisterValue(idxRegister);
        v.data = reg.data;
    } else if (addrMode == AddressMode::Absolute) {
        return cpuBase.ReadImmediateMode(OperandSize::Long);
    } else if (addrMode == AddressMode::Indirect) {
        auto &reg = cpuBase.GetRegisterValue(idxRegister);
        v = cpuBase.ReadFromMemory(szOperand, reg.data.longword);
    }
    return v;
}


std::string InstructionDecoder::ToString() const {
    if (!GetInstructionSet().contains(opClass)) {
        return ("invalid instruction");
    }
    auto desc = *GetOpDescFromClass(opClass);
    std::string opString;

    opString = desc.name;
    if (desc.features & OperandDescriptionFlags::OperandSize) {
        switch(szOperand) {
            case OperandSize::Byte :
                opString += ".b";
                break;
            case OperandSize::Word :
                opString += ".w";
                break;
            case OperandSize::DWord :
                opString += ".d";
                break;
            case OperandSize::Long :
                opString += ".l";
                break;
        }
    }

    if (desc.features & OperandDescriptionFlags::OneOperand) {
        opString += "\t";
        opString += DisasmOperand(dstAddrMode, dstRegIndex);
    }

    if (desc.features & OperandDescriptionFlags::TwoOperands) {
        opString += "\t";
        opString += DisasmOperand(dstAddrMode, dstRegIndex);
        opString += ",";
        opString += DisasmOperand(srcAddrMode, srcRegIndex);
    }

    return opString;
}

std::string InstructionDecoder::DisasmOperand(AddressMode addrMode, uint8_t regIndex) const {
    std::string opString = "";
    if (addrMode == AddressMode::Register) {
        if (regIndex > 7) {
            opString += "a" + fmt::format("{}", regIndex-8);
        } else {
            opString += "d" + fmt::format("{}", regIndex);
        }
    } else if (addrMode == AddressMode::Immediate) {
        switch(szOperand) {
            case OperandSize::Byte :
                opString += fmt::format("{:#x}",value.data.byte);
                break;
            case OperandSize::Word :
                opString += fmt::format("{:#x}",value.data.word);
                break;
            case OperandSize::DWord :
                opString += fmt::format("{:#x}",value.data.dword);
                break;
            case OperandSize::Long :
                opString += fmt::format("{:#x}",value.data.longword);
                break;
        }
    } else if (addrMode == AddressMode::Absolute) {
        opString += fmt::format("{:#x}", value.data.longword);
    } else if (addrMode == AddressMode::Indirect) {
        if (regIndex > 7) {
            opString += fmt::format("(a{})", regIndex-8);
        } else {
            opString += fmt::format("(d{})", regIndex);
        }
    }
    return opString;
}

