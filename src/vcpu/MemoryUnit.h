//
// Created by gnilk on 08.01.2024.
//

#ifndef MEMORYUNIT_H
#define MEMORYUNIT_H

#include <stdint.h>

namespace gnilk {
    namespace vcpu {
        class MemoryUnit {
        public:
            MemoryUnit() = default;
            virtual ~MemoryUnit() = default;
            uint64_t TranslateAddress(uint64_t virtualAddress);
        };
    }
}



#endif //MEMORYUNIT_H
