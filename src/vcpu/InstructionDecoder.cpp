//
// Created by gnilk on 16.12.23.
//

#include "InstructionDecoder.h"
#include <utility>
#include <fmt/core.h>

using namespace gnilk;
using namespace gnilk::vcpu;

InstructionDecoder::Ref InstructionDecoder::Create(uint64_t memoryOffset) {

    auto inst = std::make_shared<InstructionDecoder>();
    inst->memoryOffset = memoryOffset;
    inst->ofsStartInstr = 0;
    inst->ofsEndInstr = 0;
    return inst;
}

bool InstructionDecoder::Decode(CPUBase &cpu) {

    // Save the start offset
    ofsStartInstr = memoryOffset;

    opCodeByte = NextByte(cpu);
    if (opCodeByte == 0xff) {
        return false;
    }

    // Decode the op-code
    opCode =  static_cast<OperandCode>(opCodeByte);
    // check if we have this instruction defined
    if (!gnilk::vcpu::GetInstructionSet().contains(opCode)) {
        return false;
    }

    description = gnilk::vcpu::GetInstructionSet().at(opCode);

    //
    // Decode addressing
    //
    opSizeAndFamilyCode = 0;
    if (description.features & OperandDescriptionFlags::OperandSize) {
        opSizeAndFamilyCode = NextByte(cpu); //cpu.FetchByteFromInstrPtr();
        opSize = static_cast<OperandSize>(opSizeAndFamilyCode & 0x03);
        opFamily = static_cast<OperandFamily>((opSizeAndFamilyCode >> 4) & 0x03);
    }

    //
    // Decode src/dst handling
    //
    dstRegAndFlags = 0;
    srcRegAndFlags = 0;

    // we do NOT write values back to the CPU (thats part of instr. execution) but we do READ data as part of decoding
    if (description.features & OperandDescriptionFlags::OneOperand) {
        DecodeDstReg(cpu);
        value = ReadFrom(cpu, opSize, dstAddrMode, dstRelAddrMode, dstRegIndex);
    } else if (description.features & OperandDescriptionFlags::TwoOperands) {
        DecodeDstReg(cpu);
        DecodeSrcReg(cpu);
        value = ReadFrom(cpu, opSize, srcAddrMode, srcRelAddrMode, srcRegIndex);
    } else {
        // No operands
    }

    // Mark end of instruction decoding...
    ofsEndInstr = memoryOffset;
    return true;
}

void InstructionDecoder::DecodeDstReg(CPUBase &cpu) {
    dstRegAndFlags = NextByte(cpu); //cpu.FetchByteFromInstrPtr();

    // Same as below..
    dstAddrMode = static_cast<AddressMode>(dstRegAndFlags & 0x03);
    dstRelAddrMode.mode  = static_cast<RelativeAddressMode>((dstRegAndFlags & 0x0c)>>2);
    if ((dstRelAddrMode.mode == RelativeAddressMode::AbsRelative) || (dstRelAddrMode.mode == RelativeAddressMode::RegRelative)) {
        dstRelAddrMode.relativeAddress.absoulte = NextByte(cpu);
    }
    dstRegIndex = (dstRegAndFlags>>4) & 15;

}

void InstructionDecoder::DecodeSrcReg(CPUBase &cpu) {
    // Decode source flags and register index and source operand
    srcRegAndFlags = NextByte(cpu); //cpu.FetchByteFromInstrPtr();
    // decode source operand details
    srcAddrMode = static_cast<AddressMode>(srcRegAndFlags & 0x03);
    srcRelAddrMode.mode  = static_cast<RelativeAddressMode>((srcRegAndFlags & 0x0c)>>2);
    if ((srcRelAddrMode.mode == RelativeAddressMode::AbsRelative) || (srcRelAddrMode.mode == RelativeAddressMode::RegRelative)) {
        srcRelAddrMode.relativeAddress.absoulte = NextByte(cpu);
    }
    srcRegIndex = (srcRegAndFlags>>4) & 15;
}


RegisterValue InstructionDecoder::ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, RelativeAddressing relAddrMode, int idxRegister) {
    RegisterValue v = {};

    // This should be performed by instr. decoder...
    if (addrMode == AddressMode::Immediate) {
        v = cpuBase.ReadFromMemoryUnit(szOperand, memoryOffset);
        memoryOffset += ByteSizeOfOperandSize(szOperand);
    } else if (addrMode == AddressMode::Register) {
        auto &reg = cpuBase.GetRegisterValue(idxRegister, opFamily);
        v.data = reg.data;
    } else if (addrMode == AddressMode::Absolute) {
        v = cpuBase.ReadFromMemoryUnit(szOperand, memoryOffset);
        memoryOffset += ByteSizeOfOperandSize(szOperand);
    } else if (addrMode == AddressMode::Indirect) {
        auto &reg = cpuBase.GetRegisterValue(idxRegister, opFamily);
        // TODO: take relative addressing into account..
        v = cpuBase.ReadFromMemoryUnit(szOperand, reg.data.longword);
    }
    return v;
}

uint8_t InstructionDecoder::NextByte(CPUBase &cpu) {
    // Note: FetchFromRam will modifiy the address!!!!
    auto nextByte = cpu.FetchFromPhysicalRam<uint8_t>(memoryOffset);
    return nextByte;
}


std::string InstructionDecoder::ToString() const {
    if (!GetInstructionSet().contains(opCode)) {
        return ("invalid instruction");
    }
    auto desc = *GetOpDescFromClass(opCode);
    std::string opString;

    opString = desc.name;
    if (desc.features & OperandDescriptionFlags::OperandSize) {
        switch(opSize) {
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
        opString += DisasmOperand(dstAddrMode, dstRegIndex, dstRelAddrMode);
    }

    if (desc.features & OperandDescriptionFlags::TwoOperands) {
        opString += "\t";
        opString += DisasmOperand(dstAddrMode, dstRegIndex, dstRelAddrMode);
        opString += ",";
        opString += DisasmOperand(srcAddrMode, srcRegIndex, srcRelAddrMode);
    }

    return opString;
}

std::string InstructionDecoder::DisasmOperand(AddressMode addrMode, uint8_t regIndex, InstructionDecoder::RelativeAddressing relAddr) const {
    std::string opString = "";
    if (addrMode == AddressMode::Register) {
        if (regIndex > 7) {
            if (opFamily == OperandFamily::Control) {
                opString += "cr" + fmt::format("{}", regIndex-8);
            } else {
                opString += "a" + fmt::format("{}", regIndex-8);
            }
        } else {
            opString += "d" + fmt::format("{}", regIndex);
        }
    } else if (addrMode == AddressMode::Immediate) {
        switch(opSize) {
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
            opString += fmt::format("(a{}", regIndex-8);
        } else {
            opString += fmt::format("(d{}", regIndex);
        }
        // decode relative register handling...
        if (relAddr.mode == RelativeAddressMode::AbsRelative) {
            opString += fmt::format(" + {:#x}", relAddr.relativeAddress.absoulte);
        } else if (relAddr.mode == RelativeAddressMode::RegRelative) {
            auto relRegIdx = relAddr.relativeAddress.reg.index;
            opString += " + ";
            if (relRegIdx > 7) {
                opString += "a" + fmt::format("{}", relRegIdx-8);
            } else {
                opString += "d" + fmt::format("{}", relRegIdx);
            }
            if (relAddr.relativeAddress.reg.shift > 0) {
                auto shift = relAddr.relativeAddress.reg.shift;
                opString += fmt::format("<<{}", shift);
            }
        }
        opString += fmt::format(")");

    }
    return opString;
}

