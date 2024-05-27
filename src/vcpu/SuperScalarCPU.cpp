//
// Created by gnilk on 26.03.24.
//

#include "SuperScalarCPU.h"
#include <stdlib.h>
#include <functional>
#include "fmt/format.h"
#include <limits>
#include "VirtualCPU.h"

#include "InstructionSetV1/InstructionSetV1Decoder.h"
#include "InstructionSetV1/InstructionSetV1Def.h"

using namespace gnilk;
using namespace gnilk::vcpu;

bool InstructionPipeline::Tick(CPUBase &cpu) {
    tickCount++;
    if (!Update(cpu)) {
        return false;
    }
    if (pipeline[idxNextAvail].IsIdle()) {
        return BeginNext(cpu);
    }
    return true;
}
void InstructionPipeline::Flush(CPUBase &cpu) {
    fmt::println("Pipeline, flushing");
    for(auto &pipelineDecoder : pipeline) {
        // We reset the IP to the next instruction that would execute
        // FIXME: This won't work for out-of-order execution - not sure how to do that...
        if (pipelineDecoder.id == idNextExec) {
            fmt::println("  ResetIP to: {}", pipelineDecoder.ip.data.dword);
            cpu.SetInstrPtr(pipelineDecoder.ip.data.dword);
        }
        // FIXME: Internal decoder state
        //fmt::println("  id={}, state={} (forcing to idle)", pipelineDecoder.id, InstructionDecoder::StateToString(pipelineDecoder.decoder->state));
        pipelineDecoder.Reset();
    }
}

void InstructionPipeline::DbgDump() {
    fmt::println("PipeLine @ tick = {}, ID Next To Execute={}, Next decoder Index={}", tickCount, idNextExec, idxNextAvail);
    for(auto &pipelineDecoder : pipeline) {
        // FIXME: Internal decoder state
        //fmt::println("  id={}, state={}, ticks={}", pipelineDecoder.id, InstructionDecoder::StateToString(pipelineDecoder.decoder->state), pipelineDecoder.tickCount);
    }
}

bool InstructionPipeline::IsEmpty() {
    for(auto &pipelineDecoder : pipeline) {
        if (!pipelineDecoder.IsIdle()) {
            return false;
        }
    }
    return true;
}

bool InstructionPipeline::Update(CPUBase &cpu) {
    for (auto &pipelineDecoder : pipeline) {
        auto &decoder = pipelineDecoder.decoder;

        if (decoder->IsIdle()) {
            continue;
        }

        // Tick this unless it was finished (i.e. moved back to idle)
        if (!decoder->IsIdle()) {
            if (!pipelineDecoder.Tick(cpu)) {
                return false;
            }
        }

        if (CanExecute(pipelineDecoder)) {

            idNextExec = NextExecID(idNextExec);

            fmt::println("Pipeline, EXECUTE id={} (idLast={}, idNext={}), ticks={}, instr={}", pipelineDecoder.id, idLastExec, idNextExec, pipelineDecoder.tickCount, decoder->ToString());
            if (cbDecoded != nullptr) {
                cbDecoded(*decoder);
            }
            idLastExec = pipelineDecoder.id;
            //decoder.state = InstructionDecoder::State::kStateIdle;
            decoder->Reset();

            // should we 'pad' instructions up to next 32 bit boundary?
            // this would make bus-reads (or in this case L1 cache reads 32 bit - which might simplify things)
            auto alignMismatch = (cpu.GetInstrPtr().data.dword & 0x03);
            auto deltaToAlign = (4 - alignMismatch) & 0x03;
            fmt::println("Pipeline, ip={}, ofAlign={}, deltaToAlign={}", cpu.GetInstrPtr().data.dword, alignMismatch, deltaToAlign);
        }

    }
    return true;
}
size_t InstructionPipeline::NextExecID(size_t id) {
    return (id + 1) % (GNK_VCPU_PIPELINE_SIZE << 2);
}

bool InstructionPipeline::CanExecute(InstructionPipeline::PipeLineDecoder &plDecoder) {
    if (!plDecoder.IsFinished()) {
        return false;
    }
    // Enforce in-order execution
    // Note: the 'idNextExec' enforces in-order execution...
    //       if removed the instructions are executed as soon as they are properly decoded
    //       but we currently don't have any 'rules' to guard ordering so that won't probably work...
    //
    //       like;
    //          move.l d0, 0x00         <- this requires 4 ticks
    //          move.l d1, d0           <- this would be decoded in 3 ticks, but can't execute because it depends on the previous instr...
    //

    if (plDecoder.id != idNextExec) {
        // Ok, this instruction is causing a pipe-line stall, we should track this!
        return false;
    }
    return true;
}

bool InstructionPipeline::BeginNext(CPUBase &cpu) {
    // Next must be idle when we get here...
    auto &pipelineDecoder = pipeline[idxNextAvail];

    auto &decoder = pipelineDecoder.decoder;
    idxNextAvail = (idxNextAvail + 1) % GNK_VCPU_PIPELINE_SIZE;

    pipelineDecoder.id = idExec;
    pipelineDecoder.ip = cpu.GetInstrPtr();
    idExec = NextExecID(idExec);

    fmt::println("PipeLine, begin instr @ {}", cpu.GetInstrPtr().data.dword);
    ipLastFetch = cpu.GetInstrPtr();
    if (!pipelineDecoder.Tick(cpu)) {
        return false;
    }

    return true;
}


void SuperScalarCPU::QuickStart(void *ptrRam, size_t sizeOfRam) {
    CPUBase::QuickStart(ptrRam, sizeOfRam);
    // In quick-start mode we create a 'fake' timer - there is no mapping to anything in RAM...
    static TimerConfigBlock timerConfigBlock = {
            .control = {
                    .enable = 1,
                    .reset = 1,
            },
            .freqSec = 1,
            .tickCounter = 0,
    };

    AddPeripheral(CPUIntFlag::INT0, CPUKnownIntIds::kTimer0, Timer::Create(&timerConfigBlock));
}

void SuperScalarCPU::Begin(void *ptrRam, size_t sizeOfRam) {
    CPUBase::Begin(ptrRam, sizeOfRam);
    // Create the timers
    AddPeripheral(CPUIntFlag::INT0, CPUKnownIntIds::kTimer0, Timer::Create(&systemBlock->timer0));
}


void SuperScalarCPU::Reset() {
    CPUBase::Reset();
}

bool SuperScalarCPU::Tick() {
    // well - do this one we have something
    return false;
}
