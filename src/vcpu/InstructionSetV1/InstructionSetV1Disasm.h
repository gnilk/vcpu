//
// Created by gnilk on 29.05.24.
//

#ifndef VCPU_INSTRUCTIONSETV1DISASM_H
#define VCPU_INSTRUCTIONSETV1DISASM_H

#include <string>

#include "InstructionSetV1Def.h"

namespace gnilk {
    namespace vcpu {
        class InstructionSetV1Disasm {
        public:
            InstructionSetV1Disasm() = default;
            virtual ~InstructionSetV1Disasm() = default;

            // Can be made static I presume
            static std::string FromDecoded(const InstructionSetV1Def::DecoderOutput &decoderOutput);
        protected:
            static std::string DisasmOperand(const InstructionSetV1Def::DecoderOutput &decoderOutput, AddressMode addrMode, uint64_t absAddress, uint8_t regIndex, InstructionSetV1Def::RelativeAddressing relAddr);
        };
    }
}



#endif //VCPU_INSTRUCTIONSETV1DISASM_H
