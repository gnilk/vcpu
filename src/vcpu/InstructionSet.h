//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSET_H
#define VCPU_INSTRUCTIONSET_H

#include <unordered_map>
#include <initializer_list>

#include "InstructionSetDefBase.h"
#include "InstructionDecoderBase.h"
#include "InstructionSetImplBase.h"

namespace gnilk {
    namespace vcpu {
        // Was struggling with this, see:
        // https://godbolt.org/z/rbcf5KsGc
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


        // Instruction set's managed by this class - the 'id' can be used transparently as the OP code for instr.set extensions
        // id '0' is reserved as the 'no-extension' => root (this is the primary instruction set used by the CPU)
        // i.e. the CPU will fetch InstructionSet 0x00 and talk to it's decoder...
        class InstructionSetManager {
        public:
            static constexpr int kRootInstrSet = 0x00;
        public:
            virtual ~InstructionSetManager() = default;
            static InstructionSetManager &Instance();

            template<typename T>
            void SetInstructionSet() {
                RegisterExtension<T>(kRootInstrSet);
            }

            InstructionSet &GetInstructionSet();
            bool HaveInstructionSet();

            template<typename T>
            bool RegisterExtension(uint8_t extOpCode) {
                if (extensions.find(extOpCode) != extensions.end()) {
                    // Already present
                    return false;
                }
                auto instance = std::make_unique<T>();
                extensions[extOpCode] = std::move(instance);
                return true;
            }

            InstructionSet &GetExtension(uint8_t extOpCode);
            bool HaveExtension(uint8_t extOpCode);

        private:
            InstructionSetManager() = default;

            std::unordered_map<uint8_t, std::unique_ptr<InstructionSet>> extensions;
        };

    }
}

#endif //VCPU_INSTRUCTIONSET_H
