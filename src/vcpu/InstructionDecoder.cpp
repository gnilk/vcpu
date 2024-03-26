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

bool InstructionDecoder::Tick(CPUBase &cpu) {
    switch(state) {
        case State::kStateIdle :
            return ExecuteTickFromIdle(cpu);
        case State::kStateDecodeAddrMode :
            return ExecuteTickDecodeAddrMode(cpu);
        case State::kStateReadMem :
            return ExecuteTickReadMem(cpu);
        case State::kStateFinished :
            fmt::println(stderr, "InstrDecoder - tick on state 'kStateFinished' we should never reach this - should auto-go to IDLE");
            return true;
    }
    return false;
}

bool InstructionDecoder::ExecuteTickFromIdle(CPUBase &cpu) {
    memoryOffset = cpu.GetInstrPtr().data.longword;
    ofsStartInstr = 0;
    ofsEndInstr = 0;

    code.opCodeByte = NextByte(cpu);
    if (code.opCodeByte == 0xff) {
        return false;
    }

    // Decode the op-code
    code.opCode =  static_cast<OperandCode>(code.opCodeByte);
    // check if we have this instruction defined
    if (!gnilk::vcpu::GetInstructionSet().contains(code.opCode)) {
        return false;
    }

    code.description = gnilk::vcpu::GetInstructionSet().at(code.opCode);

    //
    // Decode addressing
    //
    code.opSizeAndFamilyCode = NextByte(cpu);

    if (code.description.features & OperandDescriptionFlags::OperandSize) {
        code.opSize = static_cast<OperandSize>(code.opSizeAndFamilyCode & 0x03);
        code.opFamily = static_cast<OperandFamily>((code.opSizeAndFamilyCode >> 4) & 0x03);
    } else if (code.opSizeAndFamilyCode != 0) {
        // FIXME: Raise invalid opsize here exception!
        fmt::println(stderr, "InstrDecoder, operand size not available for '{}' - must be zero, got: {}", gnilk::vcpu::GetInstructionSet().at(code.opCode).name, code.opSizeAndFamilyCode);
        return false;
    }

    //
    // Decode src/dst handling
    //
    opArgDst = {};
    opArgSrc = {};

    // we do NOT write values back to the CPU (thats part of instr. execution) but we do READ data as part of decoding
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        DecodeOperandArg(cpu, opArgDst);

        // Alignment byte - we always have 32 bits in the operand, before we read any immediate or extension bits..
        // This makes pipelining easier...
        NextByte(cpu);

    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        DecodeOperandArg(cpu, opArgDst);
        DecodeOperandArg(cpu, opArgSrc);
    } else {
        // No operands, we three trailing bytes
        NextByte(cpu);
        NextByte(cpu);
    }
    auto szComputed = ComputeInstrSize();

    cpu.AdvanceInstrPtr(szComputed);

    if ((code.description.features & OperandDescriptionFlags::OneOperand) || (code.description.features & OperandDescriptionFlags::TwoOperands)) {
        state = State::kStateDecodeAddrMode;
        return true;
    }

    state = State::kStateFinished;
    return true;
}

bool InstructionDecoder::ExecuteTickDecodeAddrMode(CPUBase &cpu) {
    DecodeOperandArgAddrMode(cpu, opArgDst);
    DecodeOperandArgAddrMode(cpu, opArgSrc);
    state = State::kStateReadMem;
    return true;
}

bool InstructionDecoder::ExecuteTickReadMem(CPUBase &cpu) {
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        value = ReadDstValue(cpu); //ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        value = ReadSrcValue(cpu); //value = ReadFrom(cpu, opSize, srcAddrMode, srcAbsoluteAddr, srcRelAddrMode, srcRegIndex);
    }
    state = State::kStateFinished;
    return true;
}

bool InstructionDecoder::Decode(CPUBase &cpu) {

    // Verify instr. pointer alignment - this not needed, but having proper instr. could make the bus more effective...
    if ((cpu.registers.instrPointer.data.longword & 0x03) != 0x00) {
        // FIXME: Raise exception here
        fmt::println(stderr, "InstrDecoder, misaligned instruction at {:x}", cpu.registers.instrPointer.data.longword);
        //return false;
    }
    // Save the start offset
    ofsStartInstr = memoryOffset;

    // BEGIN - PIPELINE TICK1, first 32 bits - and enough to compute size

    code.opCodeByte = NextByte(cpu);
    if (code.opCodeByte == 0xff) {
        return false;
    }

    // Decode the op-code
    code.opCode =  static_cast<OperandCode>(code.opCodeByte);
    // check if we have this instruction defined
    if (!gnilk::vcpu::GetInstructionSet().contains(code.opCode)) {
        return false;
    }

    code.description = gnilk::vcpu::GetInstructionSet().at(code.opCode);

    //
    // Decode addressing
    //
    code.opSizeAndFamilyCode = NextByte(cpu);

    if (code.description.features & OperandDescriptionFlags::OperandSize) {
        code.opSize = static_cast<OperandSize>(code.opSizeAndFamilyCode & 0x03);
        code.opFamily = static_cast<OperandFamily>((code.opSizeAndFamilyCode >> 4) & 0x03);
    } else if (code.opSizeAndFamilyCode != 0) {
        // FIXME: Raise invalid opsize here exception!
        fmt::println(stderr, "InstrDecoder, operand size not available for '{}' - must be zero, got: {}", gnilk::vcpu::GetInstructionSet().at(code.opCode).name, code.opSizeAndFamilyCode);
        return false;
    }

    //
    // Decode src/dst handling
    //
    opArgDst = {};
    opArgSrc = {};

    // we do NOT write values back to the CPU (thats part of instr. execution) but we do READ data as part of decoding
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        DecodeOperandArg(cpu, opArgDst);

        // Alignment byte - we always have 32 bits in the operand, before we read any immediate or extension bits..
        // This makes pipelining easier...
        NextByte(cpu);

    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        DecodeOperandArg(cpu, opArgDst);
        DecodeOperandArg(cpu, opArgSrc);
    } else {
        // No operands, we three trailing bytes
        NextByte(cpu);
        NextByte(cpu);
    }
    auto szComputed = ComputeInstrSize();
    // END PIPELINE - TICK1

    // This can now all be done in another tick!
    // However, we need a mechanism to 'lock' registers - so the CPU can wait for them to become finished...
    //
    // like:
    //   move d0, 0x01
    //   add  d0, 0x02  <- can't execute before 'd0' is released -> pipeline stall!
    //

    // BEGIN PIPELINE - TICK2
    DecodeOperandArgAddrMode(cpu, opArgDst);
    DecodeOperandArgAddrMode(cpu, opArgSrc);
    // END PIPELINE - TICK 2

    // BEGIN PIPELINE - TICK 3
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        value = ReadDstValue(cpu); //ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        value = ReadSrcValue(cpu); //value = ReadFrom(cpu, opSize, srcAddrMode, srcAbsoluteAddr, srcRelAddrMode, srcRegIndex);
    }
    // END PIPELINE - TICK 3


    // Mark end of instruction decoding...
    ofsEndInstr = memoryOffset;

    auto szReal = GetInstrSizeInBytes();
    if (szReal != szComputed) {
        fmt::println(stderr,"InstrDecoder, instr.size difference detected, computed={}, real={}", szComputed, szReal);
    }


    return true;
}

size_t InstructionDecoder::ComputeInstrSize() {
    size_t opSize = 4;
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        opSize += ComputeOpArgSize(opArgDst);
    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        opSize += ComputeOpArgSize(opArgDst);
        opSize += ComputeOpArgSize(opArgSrc);
    }
    return opSize;
}

size_t InstructionDecoder::ComputeOpArgSize(InstructionDecoder::OperandArg &opArg) {
    size_t szOpArg = 0;
    if (opArg.addrMode == AddressMode::Absolute) {
        szOpArg += sizeof(uint64_t);
    }
    if ((opArg.relAddrMode.mode == RelativeAddressMode::AbsRelative) || (opArg.relAddrMode.mode == RelativeAddressMode::RegRelative)) {
        szOpArg += sizeof(uint8_t);
    }
    if (opArg.addrMode == AddressMode::Immediate) {
        szOpArg += ByteSizeOfOperandSize(code.opSize);
    }

    return szOpArg;
}

void InstructionDecoder::DecodeOperandArg(CPUBase &cpu, InstructionDecoder::OperandArg &inOutOpArg) {
    inOutOpArg.regAndFlags = NextByte(cpu);
    inOutOpArg.addrMode = static_cast<AddressMode>(inOutOpArg.regAndFlags & 0x03);
    inOutOpArg.relAddrMode.mode  = static_cast<RelativeAddressMode>((inOutOpArg.regAndFlags & 0x0c)>>2);


    inOutOpArg.regIndex = (inOutOpArg.regAndFlags>>4) & 15;
}

void InstructionDecoder::DecodeOperandArgAddrMode(CPUBase &cpu, InstructionDecoder::OperandArg &inOutOpArg) {
    if (inOutOpArg.addrMode == AddressMode::Indirect) {
        // Do nothing - this is decoded from the data about - details will be decoded further down...
    } else if (inOutOpArg.addrMode == AddressMode::Absolute) {
        // moving to an absolute address..
        inOutOpArg.absoluteAddr = cpu.FetchFromPhysicalRam<uint64_t>(memoryOffset);
    } else if (inOutOpArg.addrMode == AddressMode::Register) {
        // nothing to do here
    } else if (inOutOpArg.addrMode == AddressMode::Immediate) {
        // do something?
    } else {
        fmt::println("Destination address mode {} not supported!", (int)inOutOpArg.addrMode);
        exit(1);
    }

    // Do this here - allows for alot of strange stuff...
    if ((inOutOpArg.relAddrMode.mode == RelativeAddressMode::AbsRelative) || (inOutOpArg.relAddrMode.mode == RelativeAddressMode::RegRelative)) {
        auto relative = NextByte(cpu);
        // This is for clarity - just assigning to 'Absolute' would be sufficient..
        if (inOutOpArg.relAddrMode.mode == RelativeAddressMode::AbsRelative) {
            inOutOpArg.relAddrMode.relativeAddress.absoulte = relative;
        } else {
            inOutOpArg.relAddrMode.relativeAddress.reg.index = (relative & 0xf0) >> 4;
            inOutOpArg.relAddrMode.relativeAddress.reg.shift = (relative & 0x0f);
        }
    }

}


// Returns a value based on src op decoding
RegisterValue InstructionDecoder::ReadSrcValue(CPUBase &cpu) {
    return ReadFrom(cpu, code.opSize, opArgSrc.addrMode, opArgSrc.absoluteAddr, opArgSrc.relAddrMode, opArgSrc.regIndex);
}

// Returns a value based on dst op decoding
RegisterValue InstructionDecoder::ReadDstValue(CPUBase &cpu) {
    return ReadFrom(cpu, code.opSize, opArgDst.addrMode, opArgDst.absoluteAddr, opArgDst.relAddrMode, opArgDst.regIndex);
//    return ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
}


RegisterValue InstructionDecoder::ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, uint64_t absAddress, RelativeAddressing relAddrMode, int idxRegister) {
    RegisterValue v = {};

    // This should be performed by instr. decoder...
    if (addrMode == AddressMode::Immediate) {
        v = cpuBase.ReadFromMemoryUnit(szOperand, memoryOffset);
        memoryOffset += ByteSizeOfOperandSize(szOperand);
    } else if (addrMode == AddressMode::Register) {
        auto &reg = cpuBase.GetRegisterValue(idxRegister, code.opFamily);
        v.data = reg.data;
    } else if (addrMode == AddressMode::Absolute) {
        v = cpuBase.ReadFromMemoryUnit(szOperand, absAddress);
        //memoryOffset += ByteSizeOfOperandSize(szOperand);
    } else if (addrMode == AddressMode::Indirect) {
        auto relativeAddrOfs = ComputeRelativeAddress(cpuBase, relAddrMode);
        auto &reg = cpuBase.GetRegisterValue(idxRegister, code.opFamily);
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
    if (!GetInstructionSet().contains(code.opCode)) {
        return ("invalid instruction");
    }
    auto desc = *GetOpDescFromClass(code.opCode);
    std::string opString;

    opString = desc.name;
    if (desc.features & OperandDescriptionFlags::OperandSize) {
        switch(code.opSize) {
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
        opString += DisasmOperand(opArgDst.addrMode, opArgDst.absoluteAddr, opArgDst.regIndex, opArgDst.relAddrMode);
    }

    if (desc.features & OperandDescriptionFlags::TwoOperands) {
        opString += "\t";
        opString += DisasmOperand(opArgDst.addrMode, opArgDst.absoluteAddr, opArgDst.regIndex, opArgDst.relAddrMode);

        opString += ",";
        opString += DisasmOperand(opArgSrc.addrMode, opArgSrc.absoluteAddr, opArgSrc.regIndex, opArgSrc.relAddrMode);

    }

    return opString;
}

std::string InstructionDecoder::DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionDecoder::RelativeAddressing relAddr) const {
    std::string opString = "";
    if (addrMode == AddressMode::Register) {
        if (regIndex > 7) {
            if (code.opFamily == OperandFamily::Control) {
                opString += "cr" + fmt::format("{}", regIndex-8);
            } else {
                opString += "a" + fmt::format("{}", regIndex-8);
            }
        } else {
            opString += "d" + fmt::format("{}", regIndex);
        }
    } else if (addrMode == AddressMode::Immediate) {
        switch(code.opSize) {
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

