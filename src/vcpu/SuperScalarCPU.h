//
// Created by gnilk on 26.03.24.
//

#ifndef VCPU_SUPERSCALARCPU_H
#define VCPU_SUPERSCALARCPU_H

#include "CPUBase.h"
#include "InstructionDecoder.h"
#include "InstructionSet.h"
#include "MemoryUnit.h"
#include "Timer.h"
#include <array>

namespace gnilk {
    namespace vcpu {
#ifndef GNK_VCPU_PIPELINE_SIZE
#define GNK_VCPU_PIPELINE_SIZE 4
#endif

        class InstructionPipeline {
        public:
            using OnInstructionDecoded = std::function<void(InstructionDecoder &decoder)>;
        public:
            InstructionPipeline() = default;
            virtual ~InstructionPipeline() = default;

            void SetInstructionDecodedHandler(OnInstructionDecoded onInstructionDecoded) {
                cbDecoded = onInstructionDecoded;
            }
            bool Tick(CPUBase &cpu);
            bool IsEmpty();
        protected:
            bool Update(CPUBase &cpu);
            bool BeginNext(CPUBase &cpu);

        private:
            OnInstructionDecoded cbDecoded = nullptr;
            size_t idxNext = 0;
            std::array<InstructionDecoder, GNK_VCPU_PIPELINE_SIZE> pipeline;
        };

        // This one uses the pipelining
        class SuperScalarCPU : public CPUBase {
        public:
            SuperScalarCPU() = default;
            virtual ~SuperScalarCPU() = default;

            void QuickStart(void *ptrRam, size_t sizeOfRam) override;
            void Begin(void *ptrRam, size_t sizeOfRam) override;
            void Reset() override;

            bool Tick();
            bool ExecuteInstruction(InstructionDecoder &decoder);
            
        protected:
            // one operand instr.
            void ExecutePushInstr(InstructionDecoder& instrDecoder);
            void ExecutePopInstr(InstructionDecoder& instrDecoder);

            // two operand instr.
            void ExecuteSysCallInstr(InstructionDecoder& instrDecoder);
            void ExecuteMoveInstr(InstructionDecoder& instrDecoder);
            void ExecuteAddInstr(InstructionDecoder& instrDecoder);
            void ExecuteSubInstr(InstructionDecoder& instrDecoder);
            void ExecuteMulInstr(InstructionDecoder& instrDecoder);
            void ExecuteDivInstr(InstructionDecoder& instrDecoder);
            void ExecuteCallInstr(InstructionDecoder& instrDecoder);
            void ExecuteRetInstr(InstructionDecoder& instrDecoder);
            void ExecuteRtiInstr(InstructionDecoder& instrDecoder);
            void ExecuteLeaInstr(InstructionDecoder& instrDecoder);
            void ExecuteLsrInstr(InstructionDecoder& instrDecoder);
            void ExecuteLslInstr(InstructionDecoder& instrDecoder);
            void ExecuteAslInstr(InstructionDecoder& instrDecoder);
            void ExecuteAsrInstr(InstructionDecoder& instrDecoder);
            void ExecuteCmpInstr(InstructionDecoder& instrDecoder);
            void ExecuteBeqInstr(InstructionDecoder& instrDecoder);
            void ExecuteBneInstr(InstructionDecoder& instrDecoder);


            void WriteToDst(InstructionDecoder& instrDecoder, const RegisterValue &v);
        private:
            InstructionPipeline pipeline;
        };
    }
}


#endif //VCPU_SUPERSCALARCPU_H
