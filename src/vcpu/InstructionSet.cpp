//
// Created by gnilk on 16.12.23.
//
#include <unordered_map>
#include <optional>
#include "InstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;


static std::unordered_map<OperandClass, OperandDescription> instructionSet = {
    {OperandClass::NOP,{.features = {} }},
    {OperandClass::RET,{.features = {} }},
{OperandClass::MOV,{.features = OperandDescriptionFlags::OperandSize |
                                                OperandDescriptionFlags::TwoOperands |
                                                OperandDescriptionFlags::Immediate |
                                                OperandDescriptionFlags::Register |
                                                OperandDescriptionFlags::Addressing}},
{OperandClass::ADD,{.features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandClass::SUB,{.features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandClass::MUL,{.features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},
{OperandClass::DIV,{.features = OperandDescriptionFlags::OperandSize |
                                              OperandDescriptionFlags::TwoOperands |
                                              OperandDescriptionFlags::Immediate |
                                              OperandDescriptionFlags::Register |
                                              OperandDescriptionFlags::Addressing}},

    // Push can be from many sources
  {OperandClass::PUSH,{.features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Register | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Addressing}},
    // Pop can only be to register...
  {OperandClass::POP,{.features = OperandDescriptionFlags::OperandSize  | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Register}},

    {OperandClass::CALL, {.features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::OneOperand | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Addressing}},
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

static std::unordered_map<std::string, OperandClass> strToOpClassMap = {
    {"move", OperandClass::MOV},
    {"add", OperandClass::ADD},
    {"sub", OperandClass::SUB},
    {"mul", OperandClass::MUL},
    {"div", OperandClass::DIV},
    {"brk", OperandClass::BRK},
    {"nop", OperandClass::NOP},
    {"call", OperandClass::CALL},
    {"push", OperandClass::PUSH},
    {"pop", OperandClass::POP},
    {"ret", OperandClass::RET},
};
std::optional<OperandClass> gnilk::vcpu::GetOperandFromStr(const std::string &str) {
    if (!strToOpClassMap.contains(str)) {
        return {};
    }
    return strToOpClassMap.at(str);
}