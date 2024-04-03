//
// Created by gnilk on 02.04.2024.
//

#ifndef VCPU_REGISTERVALUE_H
#define VCPU_REGISTERVALUE_H

#include <stdint.h>

namespace gnilk {
    namespace vcpu {
        struct RegisterValue {
            union {
                uint8_t byte;
                uint16_t word;
                uint32_t dword;
                uint64_t longword;
                void *nativePtr;
            } data;
            // Make sure this is zero when creating a register..
            RegisterValue() {
                data.longword = 0;
            }
            explicit RegisterValue(uint8_t v) { data.byte = v; }
            explicit RegisterValue(uint16_t v) { data.word = v; }
            explicit RegisterValue(uint32_t v) { data.dword = v; }
            explicit RegisterValue(uint64_t v) { data.longword = v; }
            // Is this correct???
            RegisterValue(std::initializer_list<uint64_t> lst) {
                data.longword = *lst.begin();
            }
        };
    }
}

#endif //VCPU_REGISTERVALUE_H
