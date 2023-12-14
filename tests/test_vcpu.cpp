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
}

DLL_EXPORT int test_vcpu(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_create(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_move_immediate(ITesting *t) {
    uint8_t program[]= {
        0x21,0b00001101,0x44,0x33,
        0x20,0b00001101,0x44,
        0x21,0b00001101,0x44,0x33,
        0x22,0b00001101,0x44,0x33,0x22,0x11,
        0x23,0b00001101,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,

        0x21,0b0001'1101,0x44,0x33,
        0x20,0b0010'1101,0x44,
        0x21,0b0011'1101,0x44,0x33,
        0x22,0b0100'1101,0x44,0x33,0x22,0x11,

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
