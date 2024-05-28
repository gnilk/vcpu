//
// Created by gnilk on 26.05.24.
//
#include "InstructionSet.h"
#include "InstructionSetV1/InstructionSetV1.h"
#include <memory>


using namespace gnilk::vcpu;

//
// Return the one and only instr. set manager...
//
InstructionSetManager &InstructionSetManager::Instance() {
    static InstructionSetManager glb_InstructionSetManager;
    return glb_InstructionSetManager;
}


InstructionSet &InstructionSetManager::GetInstructionSet() {
    return GetExtension(kRootInstrSet);
}

bool InstructionSetManager::HaveInstructionSet() {
    return HaveExtension(kRootInstrSet);
}

InstructionSet &InstructionSetManager::GetExtension(uint8_t extOpCode) {
    if (extensions.find(extOpCode) == extensions.end()) {
        fprintf(stderr, "ERR: No instruction set with ext '0x%.2x' found\n",extOpCode);
        exit(1);
    }
    return *extensions[extOpCode];
}

bool InstructionSetManager::HaveExtension(uint8_t extOpCode) {
    return (extensions.find(extOpCode) != extensions.end());
}
