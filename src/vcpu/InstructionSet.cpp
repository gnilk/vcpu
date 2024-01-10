//
// Created by gnilk on 16.12.23.
//
#include <unordered_map>
#include <optional>
#include "InstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;


//
// This holds the full instruction set definition
// I probably want the features to specifify valid SRC/DST combos
//
static std::unordered_map<OperandCode, OperandDescription> instructionSet = {
    {OperandCode::SYS,{.name="syscall", .features = {} }},
    {OperandCode::NOP,{.name="nop", .features = {} }},
    {OperandCode::BRK,{.name="brk", .features = {} }},
    {OperandCode::RET,{.name="ret", .features = {} }},
{OperandCode::LSL, {.name="lsl", .features = OperandDescriptionFlags::OperandSize |
                                        OperandDescriptionFlags::TwoOperands |
                                        OperandDescriptionFlags::Immediate |
                                        OperandDescriptionFlags::Register }},
{OperandCode::LSR, {.name="lsr", .features = OperandDescriptionFlags::OperandSize |
                                        OperandDescriptionFlags::TwoOperands |
                                        OperandDescriptionFlags::Immediate |
                                        OperandDescriptionFlags::Register }},
{OperandCode::ASL, {.name="asl", .features = OperandDescriptionFlags::OperandSize |
                                        OperandDescriptionFlags::TwoOperands |
                                        OperandDescriptionFlags::Immediate |
                                        OperandDescriptionFlags::Register }},
{OperandCode::ASR, {.name="asr", .features = OperandDescriptionFlags::OperandSize |
                                        OperandDescriptionFlags::TwoOperands |
                                        OperandDescriptionFlags::Immediate |
                                        OperandDescriptionFlags::Register }},
{OperandCode::LEA,{.name="lea", .features = OperandDescriptionFlags::OperandSize |
                                            OperandDescriptionFlags::TwoOperands |
                                            OperandDescriptionFlags::Immediate |
                                            OperandDescriptionFlags::Register |
                                            OperandDescriptionFlags::Addressing}},
{OperandCode::MOV,{.name="move", .features = OperandDescriptionFlags::OperandSize |
                                                OperandDescriptionFlags::TwoOperands |
                                                OperandDescriptionFlags::Immediate |
                                                OperandDescriptionFlags::Control |
                                                OperandDescriptionFlags::Register |
                                                OperandDescriptionFlags::Addressing}},
{OperandCode::ADD,{.name="add", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandCode::SUB,{.name="sub", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandCode::MUL,{.name="mul", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandCode::DIV,{.name="div", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},

    // Push can be from many sources
  {OperandCode::PUSH,{.name="push", .features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Register | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Addressing}},
    // Pop can only be to register...
  {OperandCode::POP,{.name="pop", .features = OperandDescriptionFlags::OperandSize  | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Register}},

    {OperandCode::CALL, {.name="call", .features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Register  | OperandDescriptionFlags::Addressing}},
};

const std::unordered_map<OperandCode, OperandDescription> &gnilk::vcpu::GetInstructionSet() {
    return instructionSet;
}


std::optional<OperandDescription> gnilk::vcpu::GetOpDescFromClass(OperandCode opClass) {
    if (!instructionSet.contains(opClass)) {
        return{};
    }
    return instructionSet.at(opClass);
}

// Look up table is built when an operand from str is first called
static std::unordered_map<std::string, OperandCode> strToOpClassMap;

std::optional<OperandCode> gnilk::vcpu::GetOperandFromStr(const std::string &str) {
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