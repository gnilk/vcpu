//
// Created by gnilk on 08.05.24.
//

#ifndef VCPU_RAWLINKER_H
#define VCPU_RAWLINKER_H

#include <vector>

#include "BaseLinker.h"
#include "Compiler/CompileUnit.h"

namespace gnilk {
    namespace assembler {
        class RawLinker : public BaseLinker {
        public:
            RawLinker() = default;
            virtual ~RawLinker() = default;

            bool Link(Context &context) override;
            const std::vector<uint8_t> &Data() override;
        protected:
            std::vector<uint8_t> linkedData;

        };
    }
}



#endif //VCPU_RAWLINKER_H
