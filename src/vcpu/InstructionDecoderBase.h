//
// Created by gnilk on 16.05.24.
//

#ifndef VCPU_INSTRUCTIONDECODERBASE_H
#define VCPU_INSTRUCTIONDECODERBASE_H

#include <memory>
#include <stdint.h>

#include "InstructionSetDefBase.h"

#include "Dispatch.h"

namespace gnilk {
    namespace vcpu {

        class CPUBase;  // Need forward, can't include (circular) - but I just need a reference - so forward is fine


        struct DecodedOperandBase {

        };
        struct DecodedOperandArgBase {

        };


        class InstructionDecoderBase {
        public:
            using Ref = std::shared_ptr<InstructionDecoderBase>;
        public:
            InstructionDecoderBase()  = default;
            virtual ~InstructionDecoderBase() = default;

            // We need this for case where we have multiple decoders per instr. set
            // like multiple cores, or super-scalar pipelines..

            // Single pass decoding - will call 'Reset', 'Tick' while 'IsFinished' != true
            // Returns
            //   true - decoding step was fine
            //   false - tick call returned false
            bool Decode(CPUBase &cpu);

            // Reset the decoder - generally you should change state to 'Idle'
            virtual void Reset() { }
            virtual bool Finalize(CPUBase &cpu) {
                return false;
            }

            // Tick, perform a single step
            // Returns
            //  true - step was executed ok
            //  false - some kind of error occurred
            virtual bool Tick(CPUBase &cpu) { return false; }

            virtual std::string ToString() const {
                static std::string dummy = "nothing";
                return dummy;
            }

            virtual const std::string &StateString() const {
                static std::string dummy = "nothing";
                return dummy;
            }

            // Returns
            //  true - the decoder is idle
            //  false - the decoder is not idle
            virtual bool IsIdle() { return false; }

            // Returns
            //   true - the decoder is done
            //   false - the decoder is not done
            virtual bool IsFinished() { return false; }

        protected:
            // Proxy into CPU base
            uint8_t NextByte(CPUBase &cpu);

            // FIXME: need more proxies, DecoderBase is friend of CPUBase but not the specialized decoders

        protected:
            uint64_t memoryOffset = {};
            uint64_t ofsStartInstr = {};
            uint64_t ofsEndInstr = {};
        };
    }
}



#endif //VCPU_INSTRUCTIONDECODERBASE_H
