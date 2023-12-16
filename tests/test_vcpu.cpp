//
// Created by gnilk on 14.12.23.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>

#include "VirtualCPU.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
    DLL_EXPORT int test_vcpu(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_immediate(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_reg2reg(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_immediate(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_reg2reg(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_overflow(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_push(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_pop(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_call(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_nop(ITesting *t);
    DLL_EXPORT int test_vcpu_flags_orequals(ITesting *t);

}

DLL_EXPORT int test_vcpu(ITesting *t) {
    t->CaseDepends("instr_add_immediate", "instr_move_immediate");
    t->CaseDepends("instr_add_reg2reg", "instr_move_reg2reg");
    t->CaseDepends("instr_pop","instr_push");
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_create(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_flags_orequals(ITesting *t) {
    CPUStatusFlags flags = CPUStatusFlags::None;
    flags |= CPUStatusFlags::Overflow;
    TR_ASSERT(t, flags != CPUStatusFlags::None);
    TR_ASSERT(t, flags == CPUStatusFlags::Overflow);

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
}

DLL_EXPORT int test_vcpu_instr_push(ITesting *t) {
    uint8_t program[]={
        0x70,0x00,0x01,0x43,                       // push.b 0x43
        0x70,0x01,0x01,0x42,0x00,                  // push.w 0x4200
        0x70,0x02,0x01,0x41,0x00,0x00,0x00,        // push.d 0x41000000
        0x70,0x00,0x03,                            // push.b d0
        0x70,0x01,0x13,                            // push.w d1
        0x70,0x02,0x23,                            // push.d d2
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &regs = vcpu.GetRegisters();
    regs.dataRegisters[0].data.byte = 0x89;
    regs.dataRegisters[1].data.word = 0x4711;
    regs.dataRegisters[2].data.dword = 0xdeadbeef;
    auto &stack = vcpu.GetStack();

    // Push immediate
    vcpu.Step();
    TR_ASSERT(t, !stack.empty());
    TR_ASSERT(t, stack.top().data.byte == 0x43);
    vcpu.Step();
    TR_ASSERT(t, !stack.empty());
    TR_ASSERT(t, stack.top().data.word == 0x4200);
    vcpu.Step();
    TR_ASSERT(t, !stack.empty());
    TR_ASSERT(t, stack.top().data.dword == 0x41000000);
    // Push registers
    vcpu.Step();
    TR_ASSERT(t, !stack.empty());
    TR_ASSERT(t, stack.top().data.byte == 0x89);
    vcpu.Step();
    TR_ASSERT(t, !stack.empty());
    TR_ASSERT(t, stack.top().data.word == 0x4711);
    vcpu.Step();
    TR_ASSERT(t, !stack.empty());
    TR_ASSERT(t, stack.top().data.dword == 0xdeadbeef);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_pop(ITesting *t) {
    // Must be reversed order form push...
    uint8_t program[]={
        0x80,0x02,0x03,                            // pop.d d0
        0x80,0x01,0x03,                            // pop.w d0
        0x80,0x00,0x03,                            // pop.b d0
        0x80,0x02,0x03,                            // pop.d d0
        0x80,0x01,0x13,                            // pop.w d1
        0x80,0x00,0x23,                            // pop.b d2
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &regs = vcpu.GetRegisters();
    auto &stack = vcpu.GetStack();

    // Setup stack...
    std::vector<RegisterValue> stackValues = {
        RegisterValue(uint8_t(0x43)),
        RegisterValue(uint16_t(0x4200)),
        RegisterValue(uint32_t(0x41000000)),

        RegisterValue(uint8_t(0x89)),
        RegisterValue(uint16_t(0x4711)),
        RegisterValue(uint32_t(0xdeadbeef)),
    };
    for(auto &v : stackValues) {
        stack.push(v);
    }

    // Pop and verify
    TR_ASSERT(t, !stack.empty());
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.dword == 0xdeadbeef);

    TR_ASSERT(t, !stack.empty());
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.word == 0x4711);

    TR_ASSERT(t, !stack.empty());
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x89);

    TR_ASSERT(t, !stack.empty());
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.dword == 0x41000000);

    TR_ASSERT(t, !stack.empty());
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0x4200);

    TR_ASSERT(t, !stack.empty());
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[2].data.byte == 0x43);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_call(ITesting *t) {
    // 0xf1 = NOP
    uint8_t program[]={
        0xc0,0x00,0x01,0x03,        // 0, Call IP+2   ; from en of instr -> 4+3 => 7
        0xf1,                       // 4
        0xff,                       // 5 WHALT!
        0xf1,                       // 6 <- call should go here
        0xf1,                       // 7
        0xf0,                       // 8 <- return, should be ip+1 => 5
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &instrPtr = vcpu.GetInstrPtr();
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 7);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 8);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 5);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 6);
    // We should be halted now!
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_nop(ITesting *t) {
    uint8_t program[]={
        0xf1,0xf1,0xf1,0xf1,        // 4 nop
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);

    auto &instrPtr = vcpu.GetInstrPtr();
    TR_ASSERT(t, instrPtr.data.longword == 0);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 1);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 2);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 3);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 4);

    return kTR_Pass;
}



static void DumpStatus(const VirtualCPU &cpu) {
    auto &status = cpu.GetStatusReg();
    fmt::println("CPU Stat: [{}{}{}{}{}---]",
        status.flags.carry?"C":"-",
        status.flags.overflow?"O":"-",
        status.flags.zero?"Z":"-",
        status.flags.negative?"N":"-",
        status.flags.extend?"E":"-");
}
static void DumpRegs(const VirtualCPU &cpu) {
    auto &regs = cpu.GetRegisters();
    for(int i=0;i<8;i++) {
        fmt::print("d{}=0x{:02x}  ",i,regs.dataRegisters[i].data.byte);
        if ((i & 3) == 3) {
            fmt::println("");
        }
    }
}



DLL_EXPORT int test_vcpu_instr_add_overflow(ITesting *t) {
    // Note: d0 is preloaded with 0x70 - so we should overflow after third add
    uint8_t program[]= {
        0x30,0x00,0x13,0x03,    // add.b d1, d0     // d1 = 0 + 0x70
        0x30,0x00,0x13,0x03,    // add.b d1, d0     // d1 = 0x70 + 0x70
        0x30,0x00,0x13,0x03,    // add.b d1, d0     // d1 = 0xe0 + 0x70
        0x30,0x00,0x13,0x03,    // add.b d1, d0     // d1 = ?? + 0x70
    };
    VirtualCPU vcpu;
    vcpu.Begin(program, 1024);
    auto &regs = vcpu.GetRegisters();
    auto &statusReg = vcpu.GetStatusReg();

    // preload reg d0 with 0x70
    regs.dataRegisters[0].data.word = 0x70;

    printf("Start\n");
    DumpStatus(vcpu); DumpRegs(vcpu);
    // Verify intermediate mode reading works for 8,16,32,64 bit sizes

    printf("Step 1\n");
    vcpu.Step();
    DumpStatus(vcpu); DumpRegs(vcpu);
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0x70);

    printf("Step 2\n");
    vcpu.Step();
    DumpStatus(vcpu); DumpRegs(vcpu);
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0xe0);

    printf("Step 3\n");
    vcpu.Step();
    DumpStatus(vcpu); DumpRegs(vcpu);
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0x50);
    TR_ASSERT(t, statusReg.flags.carry == true);
    TR_ASSERT(t, statusReg.flags.extend == true);
    TR_ASSERT(t, statusReg.flags.negative == false);
    TR_ASSERT(t, statusReg.flags.zero == false);

    printf("Step 4\n");
    vcpu.Step();
    DumpStatus(vcpu); DumpRegs(vcpu);
    TR_ASSERT(t, regs.dataRegisters[1].data.word == 0xc0);

    return kTR_Pass;
}
