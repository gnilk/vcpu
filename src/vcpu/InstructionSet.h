//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSET_H
#define VCPU_INSTRUCTIONSET_H

#include "InstructionSetDefBase.h"
#include "InstructionDecoderBase.h"
#include "InstructionSetImplBase.h"

namespace gnilk {
    namespace vcpu {
        struct InstructionSet {
            uint8_t extByte = 0;        // 0 means no extension
            InstructionDecoderBase &&decoder;
            InstructionSetDefBase &&definition;
            InstructionSetImplBase &&implementation;
        };
    }
}

#endif //VCPU_INSTRUCTIONSET_H
