//
// Created by gnilk on 11.01.2024.
//

#ifndef ELFLINKER_H
#define ELFLINKER_H

#include <unordered_map>
#include <vector>

#include "BaseLinker.h"
#include "CompiledUnit.h"

#include "elfio/elfio.hpp"


// FIXME: This should not be here
#include "Compiler/IdentifierRelocatation.h"

namespace gnilk {
    namespace assembler {
        class ElfLinker : public BaseLinker {
        public:
            ElfLinker() = default;
            virtual ~ElfLinker() = default;
            const std::vector<uint8_t> &Data() override;
            bool Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders) override;
        protected:
            bool WriteElf(CompiledUnit &unit);

            bool RelocateIdentifiers(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders);
            bool RelocateRelative(CompiledUnit &unit, IdentifierAddress &identifierAddr, IdentifierAddressPlaceholder &placeHolder);
            bool RelocateAbsolute(CompiledUnit &unit, IdentifierAddress &identifierAddr, IdentifierAddressPlaceholder &placeHolder);
        private:
            std::vector<uint8_t> elfData;
            ELFIO::elfio elfWriter;
        };
    }
}



#endif //ELFLINKER_H
