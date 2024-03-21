//
// Created by gnilk on 11.01.2024.
//

#ifndef ELFLINKER_H
#define ELFLINKER_H

#include <unordered_map>
#include <vector>

#include "BaseLinker.h"
#include "Compiler/CompiledUnit.h"

#include "elfio/elfio.hpp"


// FIXME: This should not be here
#include "Compiler/Identifiers.h"

namespace gnilk {
    namespace assembler {
        class ElfLinker : public BaseLinker {
        public:
            ElfLinker() = default;
            virtual ~ElfLinker() = default;
            const std::vector<uint8_t> &Data() override;
            bool LinkOld(CompiledUnit &unit, std::unordered_map<std::string, Identifier> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders) override;
        protected:
            bool WriteElf(CompiledUnit &unit);

            bool RelocateIdentifiers(CompiledUnit &unit, std::unordered_map<std::string, Identifier> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders);
            bool RelocateRelative(CompiledUnit &unit, Identifier &identifierAddr, IdentifierAddressPlaceholder::Ref &placeHolder);
            bool RelocateAbsolute(CompiledUnit &unit, Identifier &identifierAddr, IdentifierAddressPlaceholder::Ref &placeHolder);
        private:
            std::vector<uint8_t> elfData;
            ELFIO::elfio elfWriter;
        };
    }
}



#endif //ELFLINKER_H
