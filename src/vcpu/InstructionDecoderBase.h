//
// Created by gnilk on 16.05.24.
//

#ifndef VCPU_INSTRUCTIONDECODERBASE_H
#define VCPU_INSTRUCTIONDECODERBASE_H

#include <memory>
#include <stdint.h>

#include "CPUBase.h"

namespace gnilk {
    namespace vcpu {
        class InstructionDecoder;       // This is the main instruction decoder, it must be able to check the extensions
        class InstructionDecoderBase {
            friend InstructionDecoder;
        public:
            using Ref = std::shared_ptr<InstructionDecoderBase>;
        public:
            InstructionDecoderBase()  = default;
            virtual ~InstructionDecoderBase() = default;
            virtual void Reset() { }
            virtual bool Tick(CPUBase &cpu) { return false; }
        protected:
            uint8_t NextByte(CPUBase &cpu);
            virtual bool IsComplete() { return false; }

        protected:
            uint64_t memoryOffset = {};
            uint64_t ofsStartInstr = {};
            uint64_t ofsEndInstr = {};

        };
    }
}



#endif //VCPU_INSTRUCTIONDECODERBASE_H
