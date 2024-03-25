//
// Created by gnilk on 16.12.23.
//

#include "InstructionDecoder.h"
#include <utility>
#include <fmt/core.h>

using namespace gnilk;
using namespace gnilk::vcpu;

//
// FIX THIS!
// 1) - create a structure for all 'dstXYZ' and 'srcXYZ' variables
//    - change DecodeDst/SrcReg to 'DecodeDstOrSrc'
// 2) - make the decoder a state machine
// 3) -
//
//

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
        value = ReadDstValue(cpu); //ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
    } else if (description.features & OperandDescriptionFlags::TwoOperands) {
        DecodeDstReg(cpu);
        DecodeSrcReg(cpu);
        value = ReadSrcValue(cpu); //value = ReadFrom(cpu, opSize, srcAddrMode, srcAbsoluteAddr, srcRelAddrMode, srcRegIndex);
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

    // FIXME: This should come after srcRegAndFlags
    if (dstAddrMode == AddressMode::Indirect) {
        // Do nothing - this is decoded from the data about - details will be decoded further down...
    } else if (dstAddrMode == AddressMode::Absolute) {
        // moving to an absolute address..
        dstAbsoluteAddr = cpu.FetchFromPhysicalRam<uint64_t>(memoryOffset);
    } else if (dstAddrMode == AddressMode::Register) {
        // nothing to do here
    } else if (dstAddrMode == AddressMode::Immediate) {
        // do something?
    } else {
        fmt::println("Destination address mode {} not supported!", (int)dstAddrMode);
        exit(1);
    }

    // Do this here - allows for alot of strange stuff...
    if ((dstRelAddrMode.mode == RelativeAddressMode::AbsRelative) || (dstRelAddrMode.mode == RelativeAddressMode::RegRelative)) {
        auto relative = NextByte(cpu);
        // This is for clarity - just assigning to 'Absolute' would be sufficient..
        if (dstRelAddrMode.mode == RelativeAddressMode::AbsRelative) {
            dstRelAddrMode.relativeAddress.absoulte = relative;
        } else {
            dstRelAddrMode.relativeAddress.reg.index = (relative & 0xf0) >> 4;
            dstRelAddrMode.relativeAddress.reg.shift = (relative & 0x0f);
        }
    }

    dstRegIndex = (dstRegAndFlags>>4) & 15;

}

void InstructionDecoder::DecodeSrcReg(CPUBase &cpu) {
    // Decode source flags and register index and source operand
    srcRegAndFlags = NextByte(cpu); //cpu.FetchByteFromInstrPtr();
    // decode source operand details
    srcAddrMode = static_cast<AddressMode>(srcRegAndFlags & 0x03);
    srcRelAddrMode.mode  = static_cast<RelativeAddressMode>((srcRegAndFlags & 0x0c)>>2);

    // FIXME: this should after any potential dst info..

    // Switch?
    if (srcAddrMode == AddressMode::Indirect) {

    } else if (srcAddrMode == AddressMode::Absolute) {
        srcAbsoluteAddr = cpu.FetchFromPhysicalRam<uint64_t>(memoryOffset);
    } else if ((srcAddrMode == AddressMode::Immediate) || (srcAddrMode == AddressMode::Register)) {
        // Nothing to do...
    } else {
        fmt::println("Source address mode {} not supported!", (int)srcAddrMode);
        exit(1);
    }

    // Do this here - allows for alot of strange stuff...
    if ((srcRelAddrMode.mode == RelativeAddressMode::AbsRelative) || (srcRelAddrMode.mode == RelativeAddressMode::RegRelative)) {
        auto relative = NextByte(cpu);
        // This is for clarity - just assigning to 'Absolute' would be sufficient..
        if (srcRelAddrMode.mode == RelativeAddressMode::AbsRelative) {
            srcRelAddrMode.relativeAddress.absoulte = relative;
        } else {
            srcRelAddrMode.relativeAddress.reg.index = (relative & 0xf0) >> 4;
            srcRelAddrMode.relativeAddress.reg.shift = (relative & 0x0f);
        }
    }


    srcRegIndex = (srcRegAndFlags>>4) & 15;
}

// Returns a value based on src op decoding
RegisterValue InstructionDecoder::ReadSrcValue(CPUBase &cpu) {
    return ReadFrom(cpu, opSize, srcAddrMode, srcAbsoluteAddr, srcRelAddrMode, srcRegIndex);
}

// Returns a value based on dst op decoding
RegisterValue InstructionDecoder::ReadDstValue(CPUBase &cpu) {
    return ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
}


RegisterValue InstructionDecoder::ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, uint64_t absAddress, RelativeAddressing relAddrMode, int idxRegister) {
    RegisterValue v = {};

    // This should be performed by instr. decoder...
    if (addrMode == AddressMode::Immediate) {
        v = cpuBase.ReadFromMemoryUnit(szOperand, memoryOffset);
        memoryOffset += ByteSizeOfOperandSize(szOperand);
    } else if (addrMode == AddressMode::Register) {
        auto &reg = cpuBase.GetRegisterValue(idxRegister, opFamily);
        v.data = reg.data;
    } else if (addrMode == AddressMode::Absolute) {
        v = cpuBase.ReadFromMemoryUnit(szOperand, absAddress);
        //memoryOffset += ByteSizeOfOperandSize(szOperand);
    } else if (addrMode == AddressMode::Indirect) {
        auto relativeAddrOfs = ComputeRelativeAddress(cpuBase, relAddrMode);
        auto &reg = cpuBase.GetRegisterValue(idxRegister, opFamily);
        v = cpuBase.ReadFromMemoryUnit(szOperand, reg.data.longword + relativeAddrOfs);
    }
    return v;
}

uint64_t InstructionDecoder::ComputeRelativeAddress(CPUBase &cpuBase, const RelativeAddressing &relAddrMode) {
    uint64_t relativeAddrOfs = 0;
    // Break out to own function - this is also used elsewhere..
    if ((relAddrMode.mode == RelativeAddressMode::AbsRelative) || (relAddrMode.mode == RelativeAddressMode::RegRelative)) {
        if (relAddrMode.mode == RelativeAddressMode::AbsRelative) {
            relativeAddrOfs = relAddrMode.relativeAddress.absoulte;
        } else {
            // Relative handling can only come from Integer operand family!
            auto regRel = cpuBase.GetRegisterValue(relAddrMode.relativeAddress.reg.index, OperandFamily::Integer);
            relativeAddrOfs = regRel.data.byte;
            relativeAddrOfs = relativeAddrOfs << relAddrMode.relativeAddress.reg.shift;
        }
    }
    return relativeAddrOfs;
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
        opString += DisasmOperand(dstAddrMode, dstAbsoluteAddr, dstRegIndex, dstRelAddrMode);
    }

    if (desc.features & OperandDescriptionFlags::TwoOperands) {
        opString += "\t";
        opString += DisasmOperand(dstAddrMode, dstAbsoluteAddr, dstRegIndex, dstRelAddrMode);
        opString += ",";
        opString += DisasmOperand(srcAddrMode, srcAbsoluteAddr, srcRegIndex, srcRelAddrMode);
    }

    return opString;
}

std::string InstructionDecoder::DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionDecoder::RelativeAddressing relAddr) const {
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
        opString += fmt::format("({:#x})", absAddress);
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

