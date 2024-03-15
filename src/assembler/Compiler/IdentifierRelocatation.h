//
// Created by gnilk on 11.01.2024.
//

#ifndef IDENTIFIERRELOCATATION_H
#define IDENTIFIERRELOCATATION_H

#include <string>
#include <stdint.h>
#include <memory>

#include "InstructionSet.h"
#include "Linker/Segment.h"

namespace gnilk {
    namespace assembler {
        struct IdentifierAddressPlaceholder {
            using Ref = std::shared_ptr<IdentifierAddressPlaceholder>;

            std::string ident;
            Segment::Ref segment = nullptr;
            Segment::DataChunk::Ref chunk = nullptr;
            uint64_t address;
            // For relative jump
            bool isRelative;
            vcpu::OperandSize opSize;
            uint64_t ofsRelative;   // Offset to compute from..
        };

        struct IdentifierResolvePoint {
            vcpu::OperandSize opSize;
            bool isRelative;
            size_t placeholderAddress;
        };
        struct IdentifierAddress {
            Segment::Ref segment = nullptr;;
            Segment::DataChunk::Ref chunk = nullptr;
            uint64_t absoluteAddress = 0;
            // New emittor stuff
            std::vector<IdentifierResolvePoint> resolvePoints;
        };

    }
}

#endif //IDENTIFIERRELOCATATION_H
