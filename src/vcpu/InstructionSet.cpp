//
// Created by gnilk on 26.05.24.
//
#include "InstructionSet.h"

// FIXME: Until we have a class to manage the whole thing...
#include "InstructionSetV1/InstructionSetV1.h"

gnilk::vcpu::InstructionSet &gnilk::vcpu::GetInstructionSet() {
    return glb_InstructionSetV1;
}