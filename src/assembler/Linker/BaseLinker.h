//
// Created by gnilk on 11.01.2024.
//

#ifndef BASELINKER_H
#define BASELINKER_H

#include <unordered_map>
#include <vector>

#include "Compiler/Context.h"
#include "Compiler/CompiledUnit.h"

// FIXME: This should not be here
#include "Compiler/Identifiers.h"

namespace gnilk {
    namespace assembler {
        class BaseLinker {
        public:
            BaseLinker() = default;
            virtual ~BaseLinker() = default;
            virtual bool LinkOld(CompiledUnit &unit, std::unordered_map<std::string, Identifier> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders) = 0;
            virtual bool Link(Context &contex) { return false; }
            virtual const std::vector<uint8_t> &Data() = 0;
        };
    }
}



#endif //BASELINKER_H
