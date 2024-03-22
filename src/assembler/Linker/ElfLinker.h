//
// Created by gnilk on 11.01.2024.
//

#ifndef ELFLINKER_H
#define ELFLINKER_H

#include <unordered_map>
#include <vector>

#include "BaseLinker.h"
#include "Compiler/CompileUnit.h"

#include "elfio/elfio.hpp"


// FIXME: This should not be here
#include "Compiler/Identifiers.h"

namespace gnilk {
    namespace assembler {
        class ElfLinker : public BaseLinker {
        public:
            ElfLinker() = default;
            virtual ~ElfLinker() = default;

            bool Link(Context &context) override;
            const std::vector<uint8_t> &Data() override;
        protected:
            bool WriteElf(CompileUnit &unit);
        private:
            std::vector<uint8_t> elfData;
            ELFIO::elfio elfWriter;
        };
    }
}



#endif //ELFLINKER_H
