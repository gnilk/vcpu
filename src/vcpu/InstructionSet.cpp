//
// Created by gnilk on 16.12.23.
//
#include <unordered_map>
#include "InstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;


static std::unordered_map<OperandClass, OperandDescription> instructionSet = {
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
  {OperandClass::PUSH,{.features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::Register | OperandDescriptionFlags::Immediate | OperandDescriptionFlags::Addressing}},
    // Pop can only be to register...
  {OperandClass::POP,{.features = OperandDescriptionFlags::OperandSize  | OperandDescriptionFlags::Register}},
};

const std::unordered_map<OperandClass, OperandDescription> &gnilk::vcpu::GetInstructionSet() {
    return instructionSet;
}
