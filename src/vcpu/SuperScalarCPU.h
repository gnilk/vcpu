//
// Created by gnilk on 26.03.24.
//

#ifndef VCPU_SUPERSCALARCPU_H
#define VCPU_SUPERSCALARCPU_H

#include "CPUBase.h"
#include "CPUInstructionBase.h"
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

        // Very simple instruction pipe-line, we should probably add quite a few more features to this...
        // - allow out of order execution based on rules (register masks and what not)
        // - tracking 'head' so we can restart/reset/flush the pipeline..
        //   head = IP for 'next to execute'
        // - track number of 'stalls' either due to waiting for an instruction to become in-order or
        //   if a wait occurs due to dependencies
        //
        class InstructionPipeline {
        public:
            using OnInstructionDecoded = std::function<void(InstructionDecoder &decoder)>;
            struct PipeLineDecoder {
                size_t id = {};
                int tickCount = 0;
                RegisterValue ip;
                InstructionDecoder decoder;

                bool Tick(CPUBase &cpu) {
                    tickCount++;
                    return decoder.Tick(cpu);
                }
            };
        public:
            InstructionPipeline() = default;
            virtual ~InstructionPipeline() = default;

            void SetInstructionDecodedHandler(OnInstructionDecoded onInstructionDecoded) {
                cbDecoded = onInstructionDecoded;
            }
            bool Tick(CPUBase &cpu);    // Progress one tick
            bool IsEmpty();             // Check if pipeline is empty
            void Flush(CPUBase &cpu);               // Flush pipeline

            void DbgDump();
        protected:
            bool CanExecute(PipeLineDecoder &plDecoder);    // check if we are allowed to execute an instruction
            bool Update(CPUBase &cpu);      // Update the complete pipeline
            bool BeginNext(CPUBase &cpu);   // Start decoding the next instruction
            size_t NextExecID(size_t id);   // advance the Execute ID

        private:
            OnInstructionDecoded cbDecoded = nullptr;
            size_t idxNextAvail = 0;    // Index to next available decoder instance in the pipeline

            size_t idExec = 0;      // this is assigned to the decoder at when the instruction decoding is started
            size_t idNextExec = 0;  // this is used to track which instruction should be executed next
            size_t idLastExec = 0;  // debugging - just which id was last executed

            RegisterValue ipLastFetch = {};

            size_t tickCount = 0;

            std::array<PipeLineDecoder, GNK_VCPU_PIPELINE_SIZE> pipeline;
        };

        // This one uses the pipelining
        class SuperScalarCPU : public CPUInstructionBase {
        public:
            SuperScalarCPU() = default;
            virtual ~SuperScalarCPU() = default;

            void QuickStart(void *ptrRam, size_t sizeOfRam) override;
            void Begin(void *ptrRam, size_t sizeOfRam) override;
            void Reset() override;

            bool Tick();
/*
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
*/
        private:
            InstructionPipeline pipeline;
        };
    }
}


#endif //VCPU_SUPERSCALARCPU_H
