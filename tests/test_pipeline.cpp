//
// Created by gnilk on 26.03.24.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>

#include "VirtualCPU.h"

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
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    auto &regs = vcpu.GetRegisters();
    // preload reg d0 with 0x477
    regs.dataRegisters[0].data.word = 0x4711;

    InstructionPipeline pipeline;
    pipeline.SetInstructionDecodedHandler([&vcpu](InstructionDecoder &decoder){
        fmt::println("EXECUTED: {}", decoder.ToString());
    });

    while(!vcpu.IsHalted()) {
        pipeline.Tick(vcpu);
    }

    // Verify intermediate mode reading works for 8,16,32,64 bit sizes
//    vcpu.Step();
//    TR_ASSERT(t, regs.dataRegisters[1].data.word == regs.dataRegisters[0].data.word);
//    vcpu.Step();
//    TR_ASSERT(t, regs.dataRegisters[1].data.word == regs.dataRegisters[5].data.word);
//    vcpu.Step();
//    TR_ASSERT(t, regs.dataRegisters[2].data.byte == regs.dataRegisters[5].data.byte);

    return kTR_Pass;
}


