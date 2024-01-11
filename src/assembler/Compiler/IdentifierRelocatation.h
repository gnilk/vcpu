//
// Created by gnilk on 11.01.2024.
//

#ifndef IDENTIFIERRELOCATATION_H
#define IDENTIFIERRELOCATATION_H

#include <string>
#include <stdint.h>

#include "InstructionSet.h"
#include "Linker/Segment.h"

namespace gnilk {
    namespace assembler {
        struct IdentifierAddressPlaceholder {
            std::string ident;
            Segment::Ref segment;
            uint64_t address;
            // For relative jump
            bool isRelative;
            vcpu::OperandSize opSize;
            uint64_t ofsRelative;   // Offset to compute from..
        };
        struct IdentifierAddress {
            Segment::Ref segment;
            uint64_t address;
        };

    }
}

#endif //IDENTIFIERRELOCATATION_H
