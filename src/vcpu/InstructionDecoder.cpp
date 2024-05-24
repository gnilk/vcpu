//
// Created by gnilk on 16.12.23.
//

#include "InstructionDecoder.h"
#include <utility>
#include <fmt/core.h>

using namespace gnilk;
using namespace gnilk::vcpu;


const std::string &InstructionDecoder::StateToString(InstructionDecoder::State s) {
    static std::unordered_map<InstructionDecoder::State, std::string> stateNames = {
            {InstructionDecoder::State::kStateIdle, "Idle"},
            {InstructionDecoder::State::kStateDecodeAddrMode, "AddrMode"},
            {InstructionDecoder::State::kStateReadMem, "ReadMem"},
            {InstructionDecoder::State::kStateFinished, "Finished"},
            {InstructionDecoder::State::kStateDecodeExtension, "Extension"},
    };
    return stateNames[s];
}

InstructionDecoder::Ref InstructionDecoder::Create(uint64_t memoryOffset) {

    auto inst = std::make_shared<InstructionDecoder>();
    inst->memoryOffset = memoryOffset;
    inst->ofsStartInstr = 0;
    inst->ofsEndInstr = 0;
    return inst;
}

bool InstructionDecoder::Decode(CPUBase &cpu) {

    // Decode using the tick functions - this will track number of ticks for the operand
    Reset();
    while(state != State::kStateFinished) {
        if (!Tick(cpu)) {
            return false;
        }
    }
    return true;
}

void InstructionDecoder::Reset() {
    ChangeState(State::kStateIdle);
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
            //fmt::println(stderr, "InstrDecoder - tick on state 'kStateFinished' we should never reach this - should auto-go to IDLE");
            return true;
        case State::kStateDecodeExtension :
            return ExecuteTickDecodeExt(cpu);
            break;
    }
    return false;
}

//
// This is the first TICK in an instruction - it will ALWAYS read 32bit of the Instr.Ptr...
//
bool InstructionDecoder::ExecuteTickFromIdle(CPUBase &cpu) {
    memoryOffset = cpu.GetInstrPtr().data.longword;
    ofsStartInstr = memoryOffset;
    ofsEndInstr = memoryOffset;

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

    if (IsExtension(code.opCodeByte)) {
        // We will break here - just move one instruction forward..
        cpu.AdvanceInstrPtr(1);
        extDecoder = GetDecoderForInstrExt(code.opCodeByte);
        extDecoder->Reset();
        ChangeState(State::kStateDecodeExtension);
        return true;
    }


    code.description = gnilk::vcpu::GetInstructionSet().at(code.opCode);

    //
    // Decode addressing
    //
    if (code.description.features & OperandDescriptionFlags::OperandSize) {
        code.opSizeAndFamilyCode = NextByte(cpu);
        code.opSize = static_cast<OperandSize>(code.opSizeAndFamilyCode & 0x03);
        code.opFamily = static_cast<OperandFamily>((code.opSizeAndFamilyCode >> 4) & 0x03);
    }

    //
    // Decode src/dst handling
    //
    opArgDst = {};
    opArgSrc = {};

    // we do NOT write values back to the CPU (thats part of instr. execution) but we do READ data as part of decoding
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        DecodeOperandArg(cpu, opArgDst);
    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        DecodeOperandArg(cpu, opArgDst);
        DecodeOperandArg(cpu, opArgSrc);
    } else {
    }

    ofsEndInstr = memoryOffset;

    auto szComputed = ComputeInstrSize();

    // now we know how much data to consume for this instruction - advance to next - which allows us to proceed next tick
    // and the Pipeline to grab a new instruction..
    cpu.AdvanceInstrPtr(szComputed);

    if ((code.description.features & OperandDescriptionFlags::OneOperand) || (code.description.features & OperandDescriptionFlags::TwoOperands)) {
        state = State::kStateDecodeAddrMode;
        return true;
    }

    state = State::kStateFinished;
    return true;
}

// Checks if the first byte matches the extension mask
bool InstructionDecoder::IsExtension(uint8_t opCodeByte) const {
    if ((opCodeByte & OperandCodeExtensionMask) != OperandCodeExtensionMask) {
        return false;
    }
    return true;
}

//
// FIXME: Need to define WHERE extensions are registered
//
InstructionDecoderBase::Ref InstructionDecoder::GetDecoderForInstrExt(uint8_t ext) {
    return gnilk::vcpu::GetDecoderForExtension(ext);
}


//
// this is the second tick - here we decode the Operands
// relative addressing needs one byte per operand if used
// Absolute addressing will need op.size bytes will read an additional X bytes (opSize dependent) from the instr.ptr
//
bool InstructionDecoder::ExecuteTickDecodeAddrMode(CPUBase &cpu) {
    DecodeOperandArgAddrMode(cpu, opArgDst);
    if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        DecodeOperandArgAddrMode(cpu, opArgSrc);
    }
    // FIXME: Is this required?
    state = State::kStateReadMem;
    return true;
}

//
// This is the third tick, here we fetch any data from from RAM - not following the instr. pointer
//
bool InstructionDecoder::ExecuteTickReadMem(CPUBase &cpu) {
    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        value = ReadDstValue(cpu); //ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        value = ReadSrcValue(cpu); //value = ReadFrom(cpu, opSize, srcAddrMode, srcAbsoluteAddr, srcRelAddrMode, srcRegIndex);
    }
    state = State::kStateFinished;
    return true;
}

//
// Decode an extension
//
bool InstructionDecoder::ExecuteTickDecodeExt(CPUBase &cpu) {
    // FIXME: need better semantics...
    bool result = extDecoder->Tick(cpu);
    if (extDecoder->IsComplete()) {
        ChangeState(State::kStateFinished);
    }
    return result;
}

size_t InstructionDecoder::ComputeInstrSize() const {
    size_t opSize = ofsEndInstr - ofsStartInstr;    // Start here - as operands have different sizes..

    if (code.description.features & OperandDescriptionFlags::OneOperand) {
        opSize += ComputeOpArgSize(opArgDst);
    } else if (code.description.features & OperandDescriptionFlags::TwoOperands) {
        opSize += ComputeOpArgSize(opArgDst);
        opSize += ComputeOpArgSize(opArgSrc);
    }
    return opSize;
}

size_t InstructionDecoder::ComputeOpArgSize(const InstructionDecoder::OperandArg &opArg) const {
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



std::string InstructionDecoder::ToString() const {
    if (!GetInstructionSet().contains(code.opCode)) {
        // Note, we don't raise an exception - this is a helper for SW - not an actual HW type of function
        std::string invalid;
        fmt::format_to(std::back_inserter(invalid), "invalid instr. {:#x} @ {:#x}", (int)code.opCode, ofsStartInstr);
        return invalid;
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

