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
            virtual bool Decode(CPUBase &cpu);
            virtual bool Tick(CPUBase &cpu) { return false; }
            // FIXME: TEMP TEMP
            InstructionDecoderBase& GetCurrentExtDecoder() {
                return *extDecoder;
            }
            virtual std::string ToString() const {
                static std::string dummy = "nothing";
                return dummy;
            }
            // FIXME: Impl in V1
            virtual bool IsIdle() { return false; }
            // FIXME: Impl in V1
            virtual void ForceIdle() { }
            virtual bool IsComplete() { return false; }

        protected:
            uint8_t NextByte(CPUBase &cpu);

        protected:
            uint64_t memoryOffset = {};
            uint64_t ofsStartInstr = {};
            uint64_t ofsEndInstr = {};

            InstructionDecoderBase::Ref extDecoder = nullptr;


        };
    }
}



#endif //VCPU_INSTRUCTIONDECODERBASE_H
