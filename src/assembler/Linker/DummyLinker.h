//
// Created by gnilk on 11.01.2024.
//

#ifndef DUMMYLINKER_H
#define DUMMYLINKER_H

#include <unordered_map>
#include <vector>

#include "BaseLinker.h"

#include "Compiler/CompiledUnit.h"

// FIXME: This should not be here
#include "Compiler/IdentifierRelocatation.h"

namespace gnilk {
    namespace assembler {
        class DummyLinker : public BaseLinker {
        public:
            DummyLinker() = default;
            virtual ~DummyLinker() = default;

            const std::vector<uint8_t> &Data() override;
            bool LinkOld(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders) override;
            bool Link(Context &context) override;

        private:
            std::vector<uint8_t> linkedData;
        };
    }
}



#endif //DUMMYLINKER_H
