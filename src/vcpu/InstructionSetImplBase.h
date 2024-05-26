//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSETIMPLBASE_H
#define VCPU_INSTRUCTIONSETIMPLBASE_H
namespace gnilk {
    namespace vcpu {
        // This is currently not used, but the idea is to have multiple instruction set's in the future..
        // not sure I will ever bother though...
        class InstructionSetImplBase {
        public:
            InstructionSetImplBase() = default;
            virtual ~InstructionSetImplBase() = default;
            virtual bool ExecuteInstruction(CPUBase &cpu, InstructionDecoderBase &baseDecoder) {
                return false;
            }
        };
    }
}
#endif //VCPU_INSTRUCTIONSETIMPLBASE_H
