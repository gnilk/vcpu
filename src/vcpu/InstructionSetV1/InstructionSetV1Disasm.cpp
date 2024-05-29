//
// Created by gnilk on 29.05.24.
//

#include "fmt/core.h"

#include "InstructionSetV1Disasm.h"

using namespace gnilk::vcpu;

std::string InstructionSetV1Disasm::FromDecoded(const InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &instrSetDefinition = InstructionSetManager::Instance().GetInstructionSet().GetDefinition();

    if (!instrSetDefinition.GetInstructionSet().contains(decoderOutput.operand.opCode)) {
        // Note, we don't raise an exception - this is a helper for SW - not an actual HW type of function
        std::string invalid;
        fmt::format_to(std::back_inserter(invalid), "invalid instr. {:#x}", (int)decoderOutput.operand.opCode);
        return invalid;
    }
    auto desc = instrSetDefinition.GetOpDescFromClass(decoderOutput.operand.opCode);
    std::string opString;

    opString = desc->name;
    if (desc->features & OperandFeatureFlags::kFeature_OperandSize) {
        switch(decoderOutput.operand.opSize) {
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
        opString += DisasmOperand(decoderOutput, decoderOutput.opArgDst.addrMode, decoderOutput.opArgDst.absoluteAddr, decoderOutput.opArgDst.regIndex, decoderOutput.opArgDst.relAddrMode);
    }

    if (desc->features & OperandFeatureFlags::kFeature_TwoOperands) {
        opString += "\t";
        opString += DisasmOperand(decoderOutput, decoderOutput.opArgDst.addrMode, decoderOutput.opArgDst.absoluteAddr, decoderOutput.opArgDst.regIndex, decoderOutput.opArgDst.relAddrMode);

        opString += ",";
        opString += DisasmOperand(decoderOutput, decoderOutput.opArgSrc.addrMode, decoderOutput.opArgSrc.absoluteAddr, decoderOutput.opArgSrc.regIndex, decoderOutput.opArgSrc.relAddrMode);

    }

    return opString;
}



std::string InstructionSetV1Disasm::DisasmOperand(const InstructionSetV1Def::DecoderOutput &decoderOutput, AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionSetV1Def::RelativeAddressing relAddr) {
    std::string opString = "";
    if (addrMode == AddressMode::Register) {
        if (regIndex > 7) {
            if (decoderOutput.operand.opFamily == OperandFamily::Control) {
                opString += "cr" + fmt::format("{}", regIndex-8);
            } else {
                opString += "a" + fmt::format("{}", regIndex-8);
            }
        } else {
            opString += "d" + fmt::format("{}", regIndex);
        }
    } else if (addrMode == AddressMode::Immediate) {
        switch(decoderOutput.operand.opSize) {
            case OperandSize::Byte :
                opString += fmt::format("{:#x}", decoderOutput.primaryValue.data.byte);
                break;
            case OperandSize::Word :
                opString += fmt::format("{:#x}", decoderOutput.primaryValue.data.word);
                break;
            case OperandSize::DWord :
                opString += fmt::format("{:#x}", decoderOutput.primaryValue.data.dword);
                break;
            case OperandSize::Long :
                opString += fmt::format("{:#x}", decoderOutput.primaryValue.data.longword);
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
