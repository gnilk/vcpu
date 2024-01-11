//
// Created by gnilk on 11.01.2024.
//

#ifndef ELFLINKER_H
#define ELFLINKER_H

#include <unordered_map>
#include <vector>

#include "CompiledUnit.h"

// FIXME: This should not be here
#include "Compiler/IdentifierRelocatation.h"


namespace gnilk {
    namespace assembler {
        class ElfLinker {
        public:
            ElfLinker() = default;
            virtual ~ElfLinker() = default;

            bool Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders);
        protected:
            bool WriteElf(CompiledUnit &unit);

            bool RelocateIdentifiers(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders);
            bool RelocateRelative(CompiledUnit &unit, IdentifierAddress &identifierAddr, IdentifierAddressPlaceholder &placeHolder);
            bool RelocateAbsolute(CompiledUnit &unit, IdentifierAddress &identifierAddr, IdentifierAddressPlaceholder &placeHolder);
        };
    }
}



#endif //ELFLINKER_H
