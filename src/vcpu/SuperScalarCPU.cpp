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
#include "debugbreak.h"

using namespace gnilk;
using namespace gnilk::vcpu;

void InstructionPipeline::Reset() {
    for(auto &pipelineDecoder : pipelineDecoders) {
        pipelineDecoder.Reset();
    }
    idxNextAvail = 0;
    idExec = 0;
    idNextExec = 0;
    idLastExec = 0;
    tickCount = 0;
    ipLastFetch = {};       // FIXME: Verify
}

//
// Tick the instruction pipeline one step
// This will update all decoders and start ONE new...
//
bool InstructionPipeline::Tick(CPUBase &cpu) {
    tickCount++;
    fmt::println("Pipeline @ tick = {}", tickCount);
    if (!UpdatePipeline(cpu)) {
        return false;
    }

    // In case the dispatcher is a problem - we have a hard fault!
    if (!ProcessDispatcher(cpu)) {
        return false;
    }

    // Start decoding another instruction if we have one
    // Note: 'BeginNext' will perform the initial TICK to push the decoder from IDLE
    if (pipelineDecoders[idxNextAvail].IsIdle()) {
        return BeginNext(cpu);
    }
    return true;
}

//
// FIXME: This is just a placeholder - the idea is that we should reset anything in the pipeline and revert the instruction pointer back to a 'safe place' (i.e. after last-executed)
//
void InstructionPipeline::Flush(CPUBase &cpu) {
    fmt::println("Pipeline, flushing - idNext={}", idNextExec);
    for(auto &pipelineDecoder : pipelineDecoders) {
        // We reset the IP to the next instruction that would execute
        // FIXME: This won't work for out-of-order execution - not sure how to do that...
        fmt::println("  id={}, state={} (forcing to idle)", pipelineDecoder.id, pipelineDecoder.decoder->StateString());
        if (pipelineDecoder.id == idNextExec) {
            fmt::println("    ResetIP to: {}", pipelineDecoder.ip.data.dword);
            cpu.SetInstrPtr(pipelineDecoder.ip.data.dword);
        }
        pipelineDecoder.Reset();
    }
}

void InstructionPipeline::DbgDump() {
    fmt::println("PipeLine @ tick = {}, ID Next To Execute={}, Next decoder Index={}", tickCount, idNextExec, idxNextAvail);
    for(auto &pipelineDecoder : pipelineDecoders) {
        // Might not yet be assigned..
        if (pipelineDecoder.decoder == nullptr) {
            continue;
        }

        fmt::println("  id={}, state={}, ticks={}", pipelineDecoder.id, pipelineDecoder.decoder->StateString(), pipelineDecoder.tickCount);
    }
}

// Check if alla pipes are idle
bool InstructionPipeline::IsEmpty() {
    for(auto &pipelineDecoder : pipelineDecoders) {
        if (!pipelineDecoder.IsIdle()) {
            return false;
        }
    }
    return true;
}

// Update all decoders in the pipeline - IF they have anything to process...
bool InstructionPipeline::UpdatePipeline(CPUBase &cpu) {
    // Update the pipeline and push to dispatcher
    for (auto &pipelineDecoder : pipelineDecoders) {
        // Idle? - skip - 'BeginNext' will do the first 'Tick' as it initializes the decoder for the OP code
        if (pipelineDecoder.IsIdle()) {
            continue;
        }


        if (!pipelineDecoder.Tick(cpu)) {
            return false;
        }

        // Are we ready to execute - in that case - finalize
        // This is enforcing in-order execution - through 'idNextExec'
        if (CanExecute(pipelineDecoder)) {
            // Finalize and push to dispatcher..
            pipelineDecoder.decoder->Finalize(cpu);
            idNextExec = NextExecID(idNextExec);
            pipelineDecoder.Reset();
        }
    }
    return true;
}

//
// Process the dispatch queue
// Returns
//      true  - empty or instructions executed
//      false - hard fault
//
bool InstructionPipeline::ProcessDispatcher(CPUBase &cpu) {
    // Process any instruction in the dispatcher
    CPUBase::kProcessDispatchResult processResult = {};
    do {

        processResult = cpu.ProcessDispatch();
        if (processResult == CPUBase::kProcessDispatchResult::kExecOk) {
            fmt::println("  ** EXEC **: {}", cpu.GetLastExecuted());
        }
    } while(processResult == CPUBase::kProcessDispatchResult::kExecOk);

    // FIXME: Raise hard fault exception here - or let caller do it?
    if (processResult == CPUBase::kProcessDispatchResult::kNoInstrSet) {
        return false;
    } else if (processResult == CPUBase::kProcessDispatchResult::kExecFailed) {
        return false;
    }

    return true;
}

//
// Returns the next ID from an existing..  used for round-robing scheduling...
//
size_t InstructionPipeline::NextExecID(size_t id) {
    return (id + 1) % (GNK_VCPU_PIPELINE_SIZE << 2);
}


//
// Checks if a specific pipeline decoding instance can and is allowed to execute
//
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

//
// Begin's decoding on the next available decoder in the pipeline (note: caller must check if nextAvailable is free)
// Returns
//      false - the initial tick was a failure - exception raised by the decoder..
//      true  - everything when smooth
//
bool InstructionPipeline::BeginNext(CPUBase &cpu) {
    // Next must be idle when we get here...
    auto &pipelineDecoder = pipelineDecoders[idxNextAvail];

    auto &decoder = pipelineDecoder.decoder;
    idxNextAvail = (idxNextAvail + 1) % GNK_VCPU_PIPELINE_SIZE;

    pipelineDecoder.id = idExec;
    pipelineDecoder.ip = cpu.GetInstrPtr();
    idExec = NextExecID(idExec);

    fmt::println("  Begin instr @ {}", cpu.GetInstrPtr().data.dword);
    ipLastFetch = cpu.GetInstrPtr();
    if (!pipelineDecoder.Tick(cpu)) {
        return false;
    }

    return true;
}

//
// Quick start the CPU
// Note: QuickStart DOES NOT initialize the system block (i.e. memory layout block)
//
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
    pipeline.Reset();
    // NOTE: DO NOT CALL 'CPU::Reset' here since it will zero out the 'ram' ptr...
}

// Begin, this will zero out the memory and initialize the systemblock (i.e. memory layout)
void SuperScalarCPU::Begin(void *ptrRam, size_t sizeOfRam) {
    CPUBase::Begin(ptrRam, sizeOfRam);
    // Create the timers
    AddPeripheral(CPUIntFlag::INT0, CPUKnownIntIds::kTimer0, Timer::Create(&systemBlock->timer0));
    pipeline.Reset();
}


void SuperScalarCPU::Reset() {
    CPUBase::Reset();
    pipeline.Reset();
}

bool SuperScalarCPU::Tick() {
    // well - do this one we have something
    return false;
}
