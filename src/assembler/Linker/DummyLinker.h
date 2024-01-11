//
// Created by gnilk on 11.01.2024.
//

#ifndef DUMMYLINKER_H
#define DUMMYLINKER_H

#include <unordered_map>
#include <vector>

#include "BaseLinker.h"

#include "CompiledUnit.h"

// FIXME: This should not be here
#include "Compiler/IdentifierRelocatation.h"

namespace gnilk {
    namespace assembler {
        class DummyLinker : public BaseLinker {
        public:
            DummyLinker() = default;
            virtual ~DummyLinker() = default;


            bool Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders) override;
        };
    }
}



#endif //DUMMYLINKER_H
