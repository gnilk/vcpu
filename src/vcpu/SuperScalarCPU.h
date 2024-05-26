//
// Created by gnilk on 26.03.24.
//

#ifndef VCPU_SUPERSCALARCPU_H
#define VCPU_SUPERSCALARCPU_H

#include "CPUBase.h"
#include "InstructionSetV1/InstructionSetV1Impl.h"
#include "InstructionSetV1/InstructionSetV1Decoder.h"
#include "InstructionSetV1/InstructionSetV1Def.h"
#include "MemorySubSys/MemoryUnit.h"
#include "Timer.h"
#include <array>
#include <assert.h>

#include "InstructionSet.h"
#include "InstructionSetV1/InstructionSetV1.h"

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
            using OnInstructionDecoded = std::function<void(InstructionDecoderBase &decoder)>;
            struct PipeLineDecoder {
                size_t id = {};
                int tickCount = 0;
                RegisterValue ip;
                InstructionDecoderBase::Ref decoder;

                bool IsIdle() {
                    assert(decoder);
                    return decoder->IsIdle();
                }
                void ForceIdle() {
                    assert(decoder);
                    decoder->ForceIdle();
                }
                bool IsFinished() {
                    assert(decoder);
                    return decoder->IsComplete();
                }

                bool Tick(CPUBase &cpu) {
                    tickCount++;
                    // FIXME: DO NOT reference the V1 instr.set like this
                    return glb_InstructionSetV1.decoder.Tick(cpu);
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
        class SuperScalarCPU : public CPUBase {
        public:
            SuperScalarCPU() = default;
            virtual ~SuperScalarCPU() = default;

            void QuickStart(void *ptrRam, size_t sizeOfRam) override;
            void Begin(void *ptrRam, size_t sizeOfRam) override;
            void Reset() override;

            bool Tick();
        private:
            InstructionPipeline pipeline;
        };
    }
}


#endif //VCPU_SUPERSCALARCPU_H
