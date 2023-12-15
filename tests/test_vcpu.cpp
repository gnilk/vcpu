//
// Created by gnilk on 14.12.23.
//
#include <stdint.h>
#include <testinterface.h>

#include "VirtualCPU.h"

using namespace gnilk;

extern "C" {
    DLL_EXPORT int test_vcpu(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_immediate(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_reg2reg(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_immediate(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_reg2reg(ITesting *t);
}

DLL_EXPORT int test_vcpu(ITesting *t) {
    t->CaseDepends("instr_add_immediate", "instr_move_immediate");
    t->CaseDepends("instr_add_reg2reg", "instr_move_reg2reg");
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_create(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_move_immediate(ITesting *t) {
    uint8_t program[]= {
        // move.w d0, 0x4433
        0x20,0x01,0x03,0x01, 0x44,0x33,
        // move.b d0, 0x44
        0x20,0x00,0x03,0x01, 0x44,
        // move.w d0, 0x4433
        0x20,0x01,0x03,0x01, 0x44,0x33,
        // move.d d0, 0x44332211
        0x20,0x02,0x03,0x01, 0x44,0x33,0x22,0x11,
        // move.l d0, 0x8877665544332211
        0x20,0x03,0x03,0x01, 0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,

        // move.w d1, 0x4433
        0x20,0x01,0x13,0x01, 0x44,0x33,
        // move.b d2, 0x44
        0x20,0x00,0x23,0x01, 0x44,
        // move.w d3, 0x4433
        0x20,0x01,0x33,0x01, 0x44,0x33,
        // move.d d4, 0x44332211
        0x20,0x02,0x43,0x01, 0x44,0x33,0x22,0x11,
        // move.l d5, 0x8877665544332211
        0x20,0x03,0x43,0x01, 0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,

    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &regs = vcpu.GetRegisters();
    // Verify intermediate mode reading works for 8,16,32,64 bit sizes
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.word == 0x4433);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x44);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.word == 0x4433);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.dword == 0x44332211);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.longword == 0x8877665544332211);

    // Check if we can distribute across different registers
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0x4433);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[2].data.byte == 0x44);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[3].data.word == 0x4433);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[4].data.dword == 0x44332211);
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_move_reg2reg(ITesting *t) {
    uint8_t program[]= {
        0x20,0x01,0x13,0x03,    // move.w d1, d0
        0x20,0x01,0x53,0x13,    // move.w d5, d1
        0x20,0x00,0x23,0x53,    // move.b d2, d5
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &regs = vcpu.GetRegisters();
    // preload reg d0 with 0x477
    regs.dataRegisters[0].data.word = 0x4711;

    // Verify intermediate mode reading works for 8,16,32,64 bit sizes
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[1].data.word == regs.dataRegisters[0].data.word);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[1].data.word == regs.dataRegisters[5].data.word);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[2].data.byte == regs.dataRegisters[5].data.byte);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_add_immediate(ITesting *t) {
    uint8_t byteAdd[]= {
        // add.b d0, 0x44
        0x30,0x00,0x03,0x01, 0x44,
    };
    uint8_t wordAdd[]= {
        // add.w d0, 0x4433
        0x30,0x01,0x03,0x01, 0x44,0x33,
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();
    // Test 'add-value-to-zeroed-out-register'
    regs.dataRegisters[0].data.longword = 0;

    vcpu.Begin(byteAdd, 1024);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x44);

    vcpu.Reset();
    vcpu.Begin(wordAdd, 1024);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.word == 0x4433);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_add_reg2reg(ITesting *t) {
    uint8_t program[]= {
        0x30,0x01,0x13,0x03,    // add.w d1, d0     // d1 = 0 + 0x4711
        0x30,0x01,0x53,0x13,    // add.w d5, d1     // d5 = 0 + 0x4711
        0x30,0x00,0x23,0x53,    // add.b d2, d5     // d2 = 0 + 0x11
        0x30,0x00,0x23,0x01, 0x01,  // add.b d2, 0x01 // d2 = 0x12
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &regs = vcpu.GetRegisters();
    // preload reg d0 with 0x477
    regs.dataRegisters[0].data.word = 0x4711;

    // Verify intermediate mode reading works for 8,16,32,64 bit sizes
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0x4711);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0x4711);
    TR_ASSERT(t, regs.dataRegisters[5].data.word == 0x4711);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[2].data.byte == 0x11);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[2].data.byte == 0x12);

    return kTR_Pass;

    return kTR_Pass;
}
