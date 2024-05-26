//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSET_H
#define VCPU_INSTRUCTIONSET_H

#include <initializer_list>

#include "InstructionSetDefBase.h"
#include "InstructionDecoderBase.h"
#include "InstructionSetImplBase.h"

namespace gnilk {
    namespace vcpu {
        class InstructionSet {
        public:
            virtual InstructionDecoderBase &GetDecoder() = 0;
            virtual InstructionSetDefBase &GetDefinition() = 0;
            virtual InstructionSetImplBase &GetImplementation() = 0;

            virtual InstructionDecoderBase::Ref CreateDecoder() = 0;
        };
        template<typename TDec, typename TDef, typename TImpl>
        class InstructionSetInst : public InstructionSet {
        public:
            InstructionDecoderBase &GetDecoder() override {
                return decoder;
            }
            InstructionSetDefBase &GetDefinition() {
                return definition;
            };
            InstructionSetImplBase &GetImplementation() {
                return implementation;
            };

        protected:
            uint8_t extByte = 0;        // 0 means no extension
            TDec decoder;
            TDef definition;
            TImpl implementation;
        };

        // Fetch the global instruction set..
        InstructionSet &GetInstructionSet();
    }
}

#endif //VCPU_INSTRUCTIONSET_H
