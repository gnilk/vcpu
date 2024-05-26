//
// Created by gnilk on 16.12.23.
//
#include <unordered_map>
#include <optional>
#include "InstructionSetV1Def.h"
#include "Simd/SIMDInstructionDecoder.h"
#include "InstructionSetV1/InstructionSetV1Decoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;


//
// This holds the full instruction set definition
// I probably want the features to specifify valid SRC/DST combos
//
static std::unordered_map<OperandCodeBase, OperandDescriptionBase> instructionSet = {
    {OperandCode::SYS,{.name="syscall", .features = {} }},
    {OperandCode::NOP,{.name="nop", .features = {} }},
    {OperandCode::BRK,{.name="brk", .features = {} }},
    {OperandCode::RET,{.name="ret", .features = {} }},
    {OperandCode::RTI,{.name="rti", .features = {} }},
    {OperandCode::RTE,{.name="rte", .features = {} }},
{OperandCode::BEQ,{.name="beq", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize | InstructionSetV1Def::OperandDescriptionFlags::OneOperand |  InstructionSetV1Def::OperandDescriptionFlags::Immediate | InstructionSetV1Def::OperandDescriptionFlags::Branching}},
{OperandCode::BNE,{.name="bne", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize | InstructionSetV1Def::OperandDescriptionFlags::OneOperand |  InstructionSetV1Def::OperandDescriptionFlags::Immediate | InstructionSetV1Def::OperandDescriptionFlags::Branching}},

{OperandCode::LSL, {.name="lsl", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                        InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                        InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                        InstructionSetV1Def::OperandDescriptionFlags::Register }},
{OperandCode::LSR, {.name="lsr", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                        InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                        InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                        InstructionSetV1Def::OperandDescriptionFlags::Register }},
{OperandCode::ASL, {.name="asl", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                        InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                        InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                        InstructionSetV1Def::OperandDescriptionFlags::Register }},
{OperandCode::ASR, {.name="asr", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                        InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                        InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                        InstructionSetV1Def::OperandDescriptionFlags::Register }},
{OperandCode::LEA,{.name="lea", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                            InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                            InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                            InstructionSetV1Def::OperandDescriptionFlags::Register |
                                            InstructionSetV1Def::OperandDescriptionFlags::Addressing |
                                            InstructionSetV1Def::OperandDescriptionFlags::Branching}},       // <- FIXME: Rename this
{OperandCode::MOV,{.name="move", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                                InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                                InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                                InstructionSetV1Def::OperandDescriptionFlags::Control |
                                                InstructionSetV1Def::OperandDescriptionFlags::Register |
                                                InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
    {OperandCode::CMP, {.name="cmp", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                                        InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                                        InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                                        InstructionSetV1Def::OperandDescriptionFlags::Register |
                                                        InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
    {OperandCode::ADD,{.name="add", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                              InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                              InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                              InstructionSetV1Def::OperandDescriptionFlags::Register |
                                              InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
{OperandCode::SUB,{.name="sub", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                              InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                              InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                              InstructionSetV1Def::OperandDescriptionFlags::Register |
                                              InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
{OperandCode::MUL,{.name="mul", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                              InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                              InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                              InstructionSetV1Def::OperandDescriptionFlags::Register |
                                              InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
{OperandCode::DIV,{.name="div", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize |
                                              InstructionSetV1Def::OperandDescriptionFlags::TwoOperands |
                                              InstructionSetV1Def::OperandDescriptionFlags::Immediate |
                                              InstructionSetV1Def::OperandDescriptionFlags::Register |
                                              InstructionSetV1Def::OperandDescriptionFlags::Addressing}},

    // Push can be from many sources
  {OperandCode::PUSH,{.name="push", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize | InstructionSetV1Def::OperandDescriptionFlags::OneOperand | InstructionSetV1Def::OperandDescriptionFlags::Register | InstructionSetV1Def::OperandDescriptionFlags::Immediate | InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
    // Pop can only be to register...
  {OperandCode::POP,{.name="pop", .features = InstructionSetV1Def::OperandDescriptionFlags::OperandSize  | InstructionSetV1Def::OperandDescriptionFlags::OneOperand | InstructionSetV1Def::OperandDescriptionFlags::Register}},

    {OperandCode::CALL, {.name="call", .features = InstructionSetV1Def::OperandDescriptionFlags::Branching | InstructionSetV1Def::OperandDescriptionFlags::OperandSize | InstructionSetV1Def::OperandDescriptionFlags::OneOperand | InstructionSetV1Def::OperandDescriptionFlags::Immediate | InstructionSetV1Def::OperandDescriptionFlags::Register  | InstructionSetV1Def::OperandDescriptionFlags::Addressing}},
    // Extension byte - nothing, no name - not available to the assembler
    {OperandCode::SIMD, {.name="", .features = InstructionSetV1Def::OperandDescriptionFlags::Extension }},
};

const std::unordered_map<OperandCodeBase, OperandDescriptionBase> &InstructionSetV1Def::GetInstructionSet() {
    return instructionSet;
}


std::optional<OperandDescriptionBase> InstructionSetV1Def::GetOpDescFromClass(OperandCodeBase opClass) {
    if (!instructionSet.contains(opClass)) {
        return{};
    }
    return instructionSet.at(opClass);
}

// Look up table is built when an operand from str is first called
static std::unordered_map<std::string, OperandCodeBase> strToOpClassMap;

std::optional<OperandCodeBase> InstructionSetV1Def::GetOperandFromStr(const std::string &str) {
    // Lazy create on first request
    static bool haveNameOpClassTable = false;
    if (!haveNameOpClassTable) {
        for(auto [opClass, desc] : instructionSet) {
            strToOpClassMap[desc.name] = opClass;
        }
        haveNameOpClassTable = true;
    }

    if (!strToOpClassMap.contains(str)) {
        return {};
    }
    return strToOpClassMap.at(str);
}

InstructionDecoderBase::Ref gnilk::vcpu::GetDecoderForExtension(uint8_t extCode) {
    if (extCode == OperandCode::SIMD) {
        // FIXME: THIS SHOULD NOT BE HERE123!@#$
        static SIMDInstructionDecoder::Ref glbSimdDecoder  = nullptr;
        if (glbSimdDecoder == nullptr) {
            glbSimdDecoder = SIMDInstructionDecoder::Create();
        }
        return glbSimdDecoder;
    }
    return nullptr;
}