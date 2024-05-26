//
// Created by gnilk on 26.05.24.
//
#include "InstructionSet.h"
#include "InstructionSetV1/InstructionSetV1.h"

using namespace gnilk::vcpu;


gnilk::vcpu::InstructionSet &gnilk::vcpu::GetInstructionSet() {
    static InstructionSetV1 glb_InstructionSetV1;
    return glb_InstructionSetV1;
}