//
// Created by gnilk on 16.12.23.
//

//
// This defines the Instruction Decoder for the base instruction-set v1.
// The decoder can hold a single instruction. It will push it to the dispatcher when done.
// Beside actual instruction decoding the decoder also reads any data from memory.
//
// This loosely follows the https://upload.wikimedia.org/wikipedia/commons/b/b0/AMD_Bulldozer_block_diagram_%28CPU_core_block%29.png
//
//

#include "InstructionSetV1Def.h"
#include "InstructionSetV1Decoder.h"
#include <utility>
#include "fmt/core.h"

using namespace gnilk;
using namespace gnilk::vcpu;


const std::string &InstructionSetV1Decoder::StateToString(InstructionSetV1Decoder::State s) {
    static std::unordered_map<InstructionSetV1Decoder::State, std::string> stateNames = {
            {InstructionSetV1Decoder::State::kStateIdle,            "Idle"},
            {InstructionSetV1Decoder::State::kStateDecodeAddrMode,  "AddrMode"},
            {InstructionSetV1Decoder::State::kStateReadMem,         "ReadMem"},
            {InstructionSetV1Decoder::State::kStateFinished,        "Finished"},
            {InstructionSetV1Decoder::State::kStateDecodeExtension, "Extension"},
    };
    return stateNames[s];
}

InstructionSetV1Decoder::Ref InstructionSetV1Decoder::Create() {
    auto inst = std::make_shared<InstructionSetV1Decoder>();
    return inst;
}

void InstructionSetV1Decoder::Reset() {
    // Make sure to nullify this - otherwise we will call it during finalize...
    currentExtDecoder = nullptr;

    // Reset all parsing states
    code = {};
    opArgDst = {};
    opArgSrc = {};
    primaryValue = {};
    secondaryValue = {};

    ChangeState(State::kStateIdle);
}

//
//        See (for a simplified picture): https://upload.wikimedia.org/wikipedia/commons/b/b0/AMD_Bulldozer_block_diagram_%28CPU_core_block%29.png
//
bool InstructionSetV1Decoder::Tick(CPUBase &cpu) {
    bool res = false;
    switch(state) {
        case State::kStateIdle :
            res = ExecuteTickFromIdle(cpu);
            break;
        case State::kStateDecodeAddrMode :
            res = ExecuteTickDecodeAddrMode(cpu);
            break;
        case State::kStateReadMem :
            res =  ExecuteTickReadMem(cpu);
            break;
        case State::kStateTwoOpDstReadMem :
            res = ExecuteTickReadDstMem(cpu);
            break;
        case State::kStateDecodeExtension :
            res = ExecuteTickDecodeExt(cpu);
            break;
        case State::kStateFinished :
            return true;
    }
    if (state == State::kStateFinished) {
        res = Finalize(cpu);
    }
    return res;
}


void InstructionSetV1Decoder::ChangeState(State newState) {
    state = newState;
}
bool InstructionSetV1Decoder::Finalize(gnilk::vcpu::CPUBase &cpu) {
    if (IsExtension(code.opCodeByte) && (currentExtDecoder != nullptr)) {
        // Note: The extension has already automatically on changing to finished - we just check if we had a valid extension...
        return true;
    }
    return PushToDispatch(cpu);
}

// This will push the operand to the dispatcher
bool InstructionSetV1Decoder::PushToDispatch(CPUBase &cpu) {
    if (state != State::kStateFinished) {
        return false;
    }

    auto &dispatcher = cpu.GetDispatch();
    if (!dispatcher.CanInsert(sizeof(InstructionSetV1Def::DecoderOutput))) {
        return false;
    }

    InstructionSetV1Def::DecoderOutput output {
        .operand = code,
        .opArgDst = opArgDst,
        .opArgSrc = opArgSrc,
        .primaryValue = primaryValue,
        .secondaryValue = secondaryValue,
    };

    return dispatcher.Push(0, &output, sizeof(output));
}


//
// This is the first TICK in an instruction - it will ALWAYS read 32bit of the Instr.Ptr...
//
bool InstructionSetV1Decoder::ExecuteTickFromIdle(CPUBase &cpu) {
    memoryOffset = cpu.GetInstrPtr().data.longword;
    ofsStartInstr = memoryOffset;
    ofsEndInstr = memoryOffset;

    code.opCodeByte = NextByte(cpu);
    if (code.opCodeByte == 0xff) {
        return false;
    }

    auto &instrSetDefinition = InstructionSetManager::Instance().GetInstructionSet().GetDefinition();

    // Decode the op-code
    code.opCode =  static_cast<OperandCode>(code.opCodeByte);
    // check if we have this instruction defined
    if (!instrSetDefinition.GetInstructionSet().contains(code.opCode)) {
        return false;
    }

    if (IsExtension(code.opCodeByte)) {
        // We will break here - just move one instruction forward..
        cpu.AdvanceInstrPtr(1);
        // Don't have extension - exit
        if (!InstructionSetManager::Instance().HaveExtension(code.opCodeByte)) {
            fmt::println(stderr, "ERR: Invalid extension {:#X} no extension instr.set registered", code.opCodeByte);
            exit(1);
        }

        // Let's create the specific decoder for this extension - and tell it which instruction set it is assigned to
        // We must have a specific instance - in case of pipelining..
        // This allows for moving instruction set's around. In a HW specification this won't be the case but for SW emulation we can
        // swap instructions in/out and make the like  plugins if we want..
        currentExtDecoder = InstructionSetManager::Instance().GetExtension(code.opCodeByte).CreateDecoder(code.opCodeByte);
        currentExtDecoder->Reset();
        ChangeState(State::kStateDecodeExtension);
        return true;
    }


    code.features = instrSetDefinition.GetInstructionSet().at(code.opCode).features;

    //
    // Decode addressing
    //
    if (code.features & OperandFeatureFlags::kFeature_OperandSize) {
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
    if (code.features & OperandFeatureFlags::kFeature_OneOperand) {
        DecodeOperandArg(cpu, opArgDst);
    } else if (code.features & OperandFeatureFlags::kFeature_TwoOperands) {
        DecodeOperandArg(cpu, opArgDst);
        DecodeOperandArg(cpu, opArgSrc);
    } else {
    }

    ofsEndInstr = memoryOffset;

    auto szComputed = ComputeInstrSize();

    // now we know how much data to consume for this instruction - advance to next - which allows us to proceed next tick
    // and the Pipeline to grab a new instruction..
    cpu.AdvanceInstrPtr(szComputed);

    if ((code.features & OperandFeatureFlags::kFeature_OneOperand) || (code.features & OperandFeatureFlags::kFeature_TwoOperands)) {
        ChangeState(State::kStateDecodeAddrMode);
        return true;
    }

    ChangeState(State::kStateFinished);
    return true;
}

// Checks if the first byte matches the extension mask
bool InstructionSetV1Decoder::IsExtension(uint8_t opCodeByte) const {
    if ((opCodeByte & OperandCodeExtensionMask) != OperandCodeExtensionMask) {
        return false;
    }
    return true;
}


//
// this is the second tick - here we decode the Operands
// relative addressing needs one byte per operand if used
// Absolute addressing will need op.size bytes will read an additional X bytes (opSize dependent) from the instr.ptr
//
bool InstructionSetV1Decoder::ExecuteTickDecodeAddrMode(CPUBase &cpu) {
    DecodeOperandArgAddrMode(cpu, opArgDst);
    if (code.features & OperandFeatureFlags::kFeature_TwoOperands) {
        DecodeOperandArgAddrMode(cpu, opArgSrc);
    }
    ChangeState(State::kStateReadMem);
    return true;
}

//
// This is the third tick, here we fetch any data from from RAM - not following the instr. pointer
//
bool InstructionSetV1Decoder::ExecuteTickReadMem(CPUBase &cpu) {
    if (code.features & OperandFeatureFlags::kFeature_OneOperand) {
        primaryValue = ReadDstValue(cpu); //ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
    } else if (code.features & OperandFeatureFlags::kFeature_TwoOperands) {
        primaryValue = ReadSrcValue(cpu); //value = ReadFrom(cpu, opSize, srcAddrMode, srcAbsoluteAddr, srcRelAddrMode, srcRegIndex);
        if (code.features & OperandFeatureFlags::kFeature_TwoOpReadSecondary) {
            ChangeState(State::kStateTwoOpDstReadMem);
            return true;
        }
    }
    ChangeState(State::kStateFinished);
    return true;
}

//
// Some two operand instr. requires two read-mem ticks to fetch all values
//
bool InstructionSetV1Decoder::ExecuteTickReadDstMem(CPUBase &cpu) {
    secondaryValue = ReadDstValue(cpu);
    ChangeState(State::kStateFinished);
    return true;
}

//
// Decode an extension
//
bool InstructionSetV1Decoder::ExecuteTickDecodeExt(CPUBase &cpu) {
    if (currentExtDecoder == nullptr) {
        fmt::println(stderr, "ERR: No extension decoder!");
        return false;
    }
    bool result = currentExtDecoder->Tick(cpu);
    if (currentExtDecoder->IsFinished()) {
        ChangeState(State::kStateFinished);
    }
    return result;
}

size_t InstructionSetV1Decoder::ComputeInstrSize() const {
    size_t opSize = ofsEndInstr - ofsStartInstr;    // Start here - as operands have different sizes..

    if (code.features & OperandFeatureFlags::kFeature_OneOperand) {
        opSize += ComputeOpArgSize(opArgDst);
    } else if (code.features & OperandFeatureFlags::kFeature_TwoOperands) {
        opSize += ComputeOpArgSize(opArgDst);
        opSize += ComputeOpArgSize(opArgSrc);
    }
    return opSize;
}

size_t InstructionSetV1Decoder::ComputeOpArgSize(const InstructionSetV1Def::DecodedOperandArg &opArg) const {
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

void InstructionSetV1Decoder::DecodeOperandArg(CPUBase &cpu, InstructionSetV1Def::DecodedOperandArg &inOutOpArg) {
    inOutOpArg.regAndFlags = NextByte(cpu);
    inOutOpArg.addrMode = static_cast<AddressMode>(inOutOpArg.regAndFlags & 0x03);
    inOutOpArg.relAddrMode.mode  = static_cast<RelativeAddressMode>((inOutOpArg.regAndFlags & 0x0c)>>2);


    inOutOpArg.regIndex = (inOutOpArg.regAndFlags>>4) & 15;
}

void InstructionSetV1Decoder::DecodeOperandArgAddrMode(CPUBase &cpu, InstructionSetV1Def::DecodedOperandArg &inOutOpArg) {
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
        auto relAddress = ComputeRelativeAddress(cpu, inOutOpArg.relAddrMode);
        inOutOpArg.relativeAddressOfs = relAddress;
    }

}

uint64_t InstructionSetV1Decoder::ComputeRelativeAddress(CPUBase &cpuBase, const InstructionSetV1Def::RelativeAddressing &relAddrMode) const {
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



// Returns a value based on src op decoding
RegisterValue InstructionSetV1Decoder::ReadSrcValue(CPUBase &cpu) {
    return ReadFrom(cpu, code.opSize, opArgSrc.addrMode, opArgSrc.absoluteAddr, opArgSrc.relAddrMode, opArgSrc.regIndex);
}

// Returns a value based on dst op decoding
RegisterValue InstructionSetV1Decoder::ReadDstValue(CPUBase &cpu) {
    return ReadFrom(cpu, code.opSize, opArgDst.addrMode, opArgDst.absoluteAddr, opArgDst.relAddrMode, opArgDst.regIndex);
//    return ReadFrom(cpu, opSize, dstAddrMode, dstAbsoluteAddr, dstRelAddrMode, dstRegIndex);
}


RegisterValue InstructionSetV1Decoder::ReadFrom(CPUBase &cpuBase, OperandSize szOperand, AddressMode addrMode, uint64_t absAddress, InstructionSetV1Def::RelativeAddressing relAddrMode, int idxRegister) {
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




std::string InstructionSetV1Decoder::ToString() const {
    auto &instrSetDefinition = InstructionSetManager::Instance().GetInstructionSet().GetDefinition(); //glb_InstructionSetV1.definition;

    if (!instrSetDefinition.GetInstructionSet().contains(code.opCode)) {
        // Note, we don't raise an exception - this is a helper for SW - not an actual HW type of function
        std::string invalid;
        fmt::format_to(std::back_inserter(invalid), "invalid instr. {:#x} @ {:#x}", (int)code.opCode, ofsStartInstr);
        return invalid;
    }
    auto desc = instrSetDefinition.GetOpDescFromClass(code.opCode);
    std::string opString;

    opString = desc->name;
    if (desc->features & OperandFeatureFlags::kFeature_OperandSize) {
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

    if (desc->features & OperandFeatureFlags::kFeature_OneOperand) {
        opString += "\t";
        opString += DisasmOperand(opArgDst.addrMode, opArgDst.absoluteAddr, opArgDst.regIndex, opArgDst.relAddrMode);
    }

    if (desc->features & OperandFeatureFlags::kFeature_TwoOperands) {
        opString += "\t";
        opString += DisasmOperand(opArgDst.addrMode, opArgDst.absoluteAddr, opArgDst.regIndex, opArgDst.relAddrMode);

        opString += ",";
        opString += DisasmOperand(opArgSrc.addrMode, opArgSrc.absoluteAddr, opArgSrc.regIndex, opArgSrc.relAddrMode);

    }

    return opString;
}

std::string InstructionSetV1Decoder::DisasmOperand(AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionSetV1Def::RelativeAddressing relAddr) const {
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
                opString += fmt::format("{:#x}", primaryValue.data.byte);
                break;
            case OperandSize::Word :
                opString += fmt::format("{:#x}", primaryValue.data.word);
                break;
            case OperandSize::DWord :
                opString += fmt::format("{:#x}", primaryValue.data.dword);
                break;
            case OperandSize::Long :
                opString += fmt::format("{:#x}", primaryValue.data.longword);
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

