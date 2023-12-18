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
//
static std::unordered_map<OperandClass, OperandDescription> instructionSet = {
    {OperandClass::NOP,{.name="nop", .features = {} }},
    {OperandClass::BRK,{.name="brk", .features = {} }},
    {OperandClass::RET,{.name="ret", .features = {} }},
{OperandClass::LEA,{.name="lea", .features = OperandDescriptionFlags::OperandSize |
                                            OperandDescriptionFlags::TwoOperands |
                                            OperandDescriptionFlags::Immediate |
                                            OperandDescriptionFlags::Register |
                                            OperandDescriptionFlags::Addressing}},
{OperandClass::MOV,{.name="move", .features = OperandDescriptionFlags::OperandSize |
                                                OperandDescriptionFlags::TwoOperands |
                                                OperandDescriptionFlags::Immediate |
                                                OperandDescriptionFlags::Register |
                                                OperandDescriptionFlags::Addressing}},
{OperandClass::ADD,{.name="add", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandClass::SUB,{.name="sub", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandClass::MUL,{.name="mul", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandClass::DIV,{.name="div", .features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},

    // Push can be from many sources
  {OperandClass::PUSH,{.name="push", .features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Register | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Addressing}},
    // Pop can only be to register...
  {OperandClass::POP,{.name="pop", .features = OperandDescriptionFlags::OperandSize  | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Register}},

    {OperandClass::CALL, {.name="call", .features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Addressing}},
};

const std::unordered_map<OperandClass, OperandDescription> &gnilk::vcpu::GetInstructionSet() {
    return instructionSet;
}


std::optional<OperandDescription> gnilk::vcpu::GetOpDescFromClass(OperandClass opClass) {
    if (!instructionSet.contains(opClass)) {
        return{};
    }
    return instructionSet.at(opClass);
}

// Look up table is built when an operand from str is first called
static std::unordered_map<std::string, OperandClass> strToOpClassMap;

std::optional<OperandClass> gnilk::vcpu::GetOperandFromStr(const std::string &str) {
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