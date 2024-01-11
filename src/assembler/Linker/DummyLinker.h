//
// Created by gnilk on 11.01.2024.
//

#ifndef DUMMYLINKER_H
#define DUMMYLINKER_H

#include <unordered_map>
#include <vector>

#include "CompiledUnit.h"

// FIXME: This should not be here
#include "Compiler/IdentifierRelocatation.h"

namespace gnilk {
    namespace assembler {
        class DummyLinker {
        public:
            DummyLinker() = default;
            virtual ~DummyLinker() = default;


            bool Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders);
        };
    }
}



#endif //DUMMYLINKER_H
