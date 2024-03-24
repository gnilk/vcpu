//
// Created by gnilk on 11.01.2024.
//

#ifndef BASELINKER_H
#define BASELINKER_H

#include <unordered_map>
#include <vector>
#include <memory>

#include "Compiler/Context.h"
#include "Compiler/CompileUnit.h"

// FIXME: This should not be here
#include "Compiler/Identifiers.h"

namespace gnilk {
    namespace assembler {
        class BaseLinker {
        public:
            using Ref = std::shared_ptr<BaseLinker>;
        public:
            BaseLinker() = default;
            virtual ~BaseLinker() = default;
            virtual bool Link(Context &contex) { return false; }
            virtual const std::vector<uint8_t> &Data() = 0;
        };
    }
}



#endif //BASELINKER_H
