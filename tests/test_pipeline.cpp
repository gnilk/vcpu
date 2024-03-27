//
// Created by gnilk on 26.03.24.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "SuperScalarCPU.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
DLL_EXPORT int test_pipeline(ITesting *t);
DLL_EXPORT int test_pipeline_instr_move_reg2reg(ITesting *t);
}
DLL_EXPORT int test_pipeline(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_pipeline_instr_move_reg2reg(ITesting *t) {
    uint8_t program[]= {
            0x20,0x01,0x13,0x03,    // move.w d1, d0
            0x20,0x01,0x53,0x13,    // move.w d5, d1
            0x20,0x00,0x23,0x53,    // move.b d2, d5
            0x00, 0x00, 0x00, 0x00, // brk
    };
    SuperScalarCPU cpu;
    cpu.QuickStart(program, 1024);
    auto &regs = cpu.GetRegisters();
    // preload reg d0 with 0x477
    regs.dataRegisters[0].data.word = 0x4711;

    InstructionPipeline pipeline;
    pipeline.SetInstructionDecodedHandler([&cpu](InstructionDecoder &decoder){
        cpu.ExecuteInstruction(decoder);
    });

    // This currently creates an out-of-order execution...
    while(!cpu.IsHalted()) {
        pipeline.DbgDump();
        pipeline.Tick(cpu);
    }
    pipeline.Flush(cpu);
    fmt::println("******** done ********");
    pipeline.DbgDump();
    TR_ASSERT(t, pipeline.IsEmpty());

    return kTR_Pass;
}


