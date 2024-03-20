//
// Created by gnilk on 20.12.23.
//

#ifndef COMPILEDUNIT_H
#define COMPILEDUNIT_H

#include <string>
#include <vector>
#include <stdint.h>

#include "InstructionSet.h"
#include "Segment.h"

namespace gnilk {
    namespace assembler {
        class Context;
        // This should represent a single compiler unit
        // Segment handling should be moved out from here -> they belong to the context as they are shared between compiled units
        class CompiledUnit {
        public:
            CompiledUnit() = default;
            virtual ~CompiledUnit() = default;

            void Clear();
            size_t Write(Context &context, const std::vector<uint8_t> &data);
//            bool WriteByte(Context &context, uint8_t byte);
//            void ReplaceAt(Context &context, uint64_t offset, uint64_t newValue);
//            void ReplaceAt(Context &context, uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize);

            uint64_t GetCurrentWriteAddress(Context &context);
            // temp?
            const std::vector<uint8_t> &Data(Context &context);
        private:
        };
    }
}

#endif //COMPILEDUNIT_H
