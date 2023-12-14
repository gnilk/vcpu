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
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    vcpu.Step();
    auto &regs = vcpu.GetRegisters();
    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x44);
    return kTR_Pass;
}
