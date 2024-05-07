//
// Created by gnilk on 14.12.23.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>

#include "VirtualCPU.h"

using namespace gnilk;
using namespace gnilk::vcpu;

static void DumpStatus(const VirtualCPU &cpu);
static void DumpRegs(const VirtualCPU &cpu);


extern "C" {
    DLL_EXPORT int test_vcpu(ITesting *t);
    DLL_EXPORT int test_vcpu_readwrite_ram(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_immediate(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_reg2reg(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_relative(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_cntrl(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_absolute(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_move_indirect(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_immediate(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_reg2reg(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_add_overflow(ITesting *t);

    DLL_EXPORT int test_vcpu_instr_push(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_pop(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_call(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_nop(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_lea(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_lsr(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_lsl(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_asr(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_asl(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_cmp(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_cmp_absolute(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_beq(ITesting *t);
    DLL_EXPORT int test_vcpu_instr_bne(ITesting *t);
    DLL_EXPORT int test_vcpu_halt(ITesting *t);
    DLL_EXPORT int test_vcpu_flags_orequals(ITesting *t);
    DLL_EXPORT int test_vcpu_disasm(ITesting *t);

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

DLL_EXPORT int test_vcpu_readwrite_ram(ITesting *t) {
    uint8_t ram[128] = {0x01,0x02,0x03,0x04};
    VirtualCPU vcpu;
    vcpu.QuickStart(ram, 1024);
    auto readReg = vcpu.ReadFromMemoryUnit(OperandSize::Byte, 0x00);
    TR_ASSERT(t, readReg.data.byte == 0x01);
    readReg = vcpu.ReadFromMemoryUnit(OperandSize::Word, 0x00);
    TR_ASSERT(t, readReg.data.word == 0x0102);
    readReg = vcpu.ReadFromMemoryUnit(OperandSize::DWord, 0x00);
    TR_ASSERT(t, readReg.data.dword == 0x01020304);


    RegisterValue writeReg = {};
    writeReg.data.byte = 0xab;
    vcpu.WriteToMemoryUnit(OperandSize::Byte, 0x00, writeReg);
    TR_ASSERT(t, ram[0] == 0xab);
    memset(ram,0, sizeof(ram));
    writeReg.data.word = 0x0102;
    vcpu.WriteToMemoryUnit(OperandSize::Word, 0x00, writeReg);
    TR_ASSERT(t, (ram[0] == 0x01) && (ram[1] == 0x02));
    memset(ram,0, sizeof(ram));
    writeReg.data.dword = 0x01020304;
    vcpu.WriteToMemoryUnit(OperandSize::DWord, 0x00, writeReg);
    TR_ASSERT(t, (ram[0] == 0x01) && (ram[1] == 0x02) && (ram[2] == 0x03) && (ram[3] == 0x04));

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
    vcpu.QuickStart(program, 1024);
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
    vcpu.QuickStart(program, 1024);
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

DLL_EXPORT int test_vcpu_instr_move_relative(ITesting *t) {
    uint8_t program[]= {
        // not quite sure how I should store this addressing...

        // op code = 0x20,  -> move
        // op size = 0x01,  -> word
        // dstRegMode = 0x03,
        // srcRegMode = 0x84,   => RRRR | MMMM => Reg = 8, Mode = 8 => b0100 => 01|00 => RelAddrMode = 01 = RegRelative, 00 = Indirect
        // relAddrMode = 0x30   => RegRelative => RRRR | SSSS, R = RegIndex, S => Shift
        0x20,0x01,0x03,0x84,0x30,        // move.w d0, (a0+d3)
        0x20,0x01,0x03,0x84,0x32,        // move.w d0, (a0+d3<<2)
        0x20,0x01,0x03,0x84,0x33,        // move.w d0, (a0+d3<<3)
        0x20,0x01,0x03,0x84,0x37,        // move.w d0, (a0+d3<<7)
        0x20,0x01,0x03,0x88,0x47,        // move.w d0, (a0+0x47)

        // this is a side-effect of the
        // op code = 0x20 -> move
        // op size = 0x01 -> word
        // dstRegMode = 0x94 => a1 + d4
        // srcRegMode = 0x84 = a0 + d3

        // dstRelMode = 0x43 => d4<<3
        // srcRelMode = 0x30 = d3<<1
        0x20,0x01,0x94,0x84,0x43,0x31,        // move.w (a1+d4<<3), (a0+d3<<1)

        0x20,0x01,0x03,0x80,             // move.w d0, (a0)
        0x00                                // brk
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    auto &regs = vcpu.GetRegisters();
    // preload reg d0 with 0x477
    regs.dataRegisters[0].data.word = 0x4711;
    regs.addressRegisters[0].data.longword = 0x05;  // read from address 5...

    int instrCounter = 0;
    while(!vcpu.IsHalted()) {
        vcpu.Step();
        fmt::println("{}", vcpu.GetLastDecodedInstr()->ToString());
        instrCounter++;
    }

    return kTR_Pass;

    return kTR_Pass;
}
DLL_EXPORT int test_vcpu_instr_move_cntrl(ITesting *t) {
    uint8_t program[]= {
        0x20,0x13,0x83,0x03,    // move.l cr0, d0
        0x20,0x13,0x03,0x83,    // move.l d0, cr0
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    auto &regs = vcpu.GetRegisters();
    // preload reg d0 with 0x477
    regs.dataRegisters[0].data.longword = 0x4711;

    // Verify intermediate mode reading works for 8,16,32,64 bit sizes
    vcpu.Step();
    TR_ASSERT(t, regs.cntrlRegisters.array[0].data.longword == regs.dataRegisters[0].data.longword);
    // Reset and read back..
    regs.dataRegisters[0].data.longword = 0x0;
    vcpu.Step();
    TR_ASSERT(t, regs.cntrlRegisters.array[0].data.longword == regs.dataRegisters[0].data.longword);

    fmt::println("{}", vcpu.GetLastDecodedInstr()->ToString());

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_move_absolute(ITesting *t) {
    uint8_t program[]={
            // op code = 0x20 => move
            // op size = 0x03 => long
            // dst reg mode = 0x02 => absolute
            // [value]
            // src reg mode = 0x01 => immediate
            // [value]
            //
        0x20, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,   // move.l (0x00000015),0x01
        0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // this should be addres 0x15
      // 0     1     2     3     4     5     6     7     8     9     10   11   12    13    14    15    16    17    18   19
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    vcpu.Step();
    uint64_t address = 0x15;
    auto value = vcpu.ReadFromMemoryUnit(OperandSize::Long, address);
    TR_ASSERT(t, 0x01 == value.data.longword);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_move_indirect(ITesting *t) {
    uint8_t program[] = {
            0x28,0x03,0x83,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,        // lea a0, 0x11
            0x20,0x00,0x03,0x80,                                                // move.b d0, (a0)
            0x00,                       // 0x10 BRK
            0xf1,                       // 0x17 <- we will read this value in the second instr..
    };

    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();

    vcpu.QuickStart(program, 1024);
    TR_ASSERT(t, vcpu.Step());
    TR_ASSERT(t, regs.addressRegisters[0].data.longword == 0x11);

    vcpu.Step();
    TR_ASSERT(t, regs.addressRegisters[0].data.longword == 0x11);
    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0xf1);

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

    vcpu.QuickStart(byteAdd, 1024);
    vcpu.Step();
    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x44);

    vcpu.SetInstrPtr(0);
    vcpu.QuickStart(wordAdd, 1024);
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
    vcpu.QuickStart(program, 1024);
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
        0x70,0x00,0x01, 0x43,                       // push.b 0x43
        0x70,0x01,0x01, 0x42,0x00,                  // push.w 0x4200
        0x70,0x02,0x01, 0x41,0x00,0x00,0x00,        // push.d 0x41000000
        0x70,0x00,0x03,                            // push.b d0
        0x70,0x01,0x13,                            // push.w d1
        0x70,0x02,0x23,                            // push.d d2
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
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
    vcpu.QuickStart(program, 1024);
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
        0xc0,0x00,0x01,0x02,        // 0, Call IP+2   ; from en of instr -> 4+3 => 7
        0xf1,                       // 4 <- return address
        0x00,                       // 5 HALT!
        0xf1,                       // 6 <- call should go here
        0xf1,                       // 7
        0xf0,                       // 8 <- return, should be ip+1 => 5
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    auto &instrPtr = vcpu.GetInstrPtr();
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 6);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 7);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 8);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 4);
    vcpu.Step();
    TR_ASSERT(t, instrPtr.data.longword == 5);
    vcpu.Step();
    // We should be halted now!
    TR_ASSERT(t, vcpu.IsHalted());
    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_nop(ITesting *t) {
    // FIXME: We should consider this to be a implicit instruction
    uint8_t program[]={
        0xf1,
        0xf1,
        0xf1,
        0xf1,
        0x00,
    };
    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);

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

DLL_EXPORT int test_vcpu_instr_lea(ITesting *t) {
    // Lea uses 'immediate' mode addressing - since the address is fixed - absolute would cause a ready of the absolute address specified
    // op code =  0x28 => LEA
    // op size =  0x03 => long word
    // dst op  => 0x83 => IIII | RRMM => I (register) = a0, RR (rel.addr.mode) = 0, MM => 3 (Register)
    // src op  => 0x01 => IIII | RRMM => I (register) = - , RR (rel.addr.mode) = 0, MM => 1 (Immediate)
    uint8_t program[]={
        0x28, 0x03, 0x83, 0x01, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11        // lea a0, 0x8877665544332211
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();

    // Problem here; LEA will encode address mode 'absolute', which is wrong - it should be 'immediate'.

    vcpu.QuickStart(program, 1024);
    auto res = vcpu.Step();
    TR_ASSERT(t, res);

    TR_ASSERT(t, regs.addressRegisters[0].data.longword == 0x8877665544332211);

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
    vcpu.QuickStart(program, 1024);
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

DLL_EXPORT int test_vcpu_instr_lsr(ITesting *t) {
    uint8_t program[]= {
        // 0xB3 - opcode, LSR
        // 0x00 - OP Size, 0 = byte
        // 0x03 - DstRegMode, RRRR|MMMM = 0000 | 0011 => d0 | 3 => 3 = register
        // 0x01 - SrcRegMode, 0000|MMMM = 0000 | 0001 => xx | 1 => Immediate
        // 0x01 - Immediate value (0x01)

        0xE3,0x00,0x03,0x01,0x01    // lsr.b d0, #1
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();

    vcpu.QuickStart(program, 1024);
    regs.dataRegisters[0].data.byte = 0x40;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);

    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x20);

    return kTR_Pass;

}

DLL_EXPORT int test_vcpu_instr_lsl(ITesting *t) {
    uint8_t program[]= {
        // 0xB3 - opcode, LSR
        // 0x00 - OP Size, 0 = byte
        // 0x03 - DstRegMode, RRRR|MMMM = 0000 | 0011 => d0 | 3 => 3 = register
        // 0x01 - SrcRegMode, 0000|MMMM = 0000 | 0001 => xx | 1 => Immediate
        // 0x01 - Immediate value (0x01)

        0xE5,0x00,0x03,0x01,0x01    // lsl.b d0, #1
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();

    vcpu.QuickStart(program, 1024);
    // we are shifting the byte - this should not affect other parts of the register...
    regs.dataRegisters[0].data.longword = 0xaabbaabbaa;
    regs.dataRegisters[0].data.byte = 0x20;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);

    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x40);

    return kTR_Pass;

}

DLL_EXPORT int test_vcpu_instr_asr(ITesting *t) {
    uint8_t program[]= {
        // 0xE4 - opcode, ASR
        // 0x00 - OP Size, 0 = byte
        // 0x03 - DstRegMode, RRRR|MMMM = 0000 | 0011 => d0 | 3 => 3 = register
        // 0x01 - SrcRegMode, 0000|MMMM = 0000 | 0001 => xx | 1 => Immediate
        // 0x01 - Immediate value (0x01)

        0xE4,0x00,0x03,0x01,0x01,    // asr.b d0, #1
        0xE4,0x00,0x03,0x01,0x03    // asr.b d0, #3
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();
    auto &status = vcpu.GetStatusReg();

    vcpu.QuickStart(program, 1024);
    regs.dataRegisters[0].data.byte = 0x82;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);
    // MSB should be set on arithmetic shift...
    TR_ASSERT(t, (regs.dataRegisters[0].data.byte & 0x80) != 0);
    TR_ASSERT(t, status.flags.negative);
    TR_ASSERT(t, !status.flags.carry);
    TR_ASSERT(t, !status.flags.extend);
    TR_ASSERT(t, !status.flags.zero);

    regs.dataRegisters[0].data.byte = 0x67;
    res = vcpu.Step();      // asr.b d0,3
    TR_ASSERT(t, res);
    // MSB should be set on arithmetic shift...
    TR_ASSERT(t, (regs.dataRegisters[0].data.byte & 0x80) == 0);
    TR_ASSERT(t, !status.flags.negative);
    TR_ASSERT(t, status.flags.carry);
    TR_ASSERT(t, status.flags.extend);
    TR_ASSERT(t, !status.flags.zero);

    return kTR_Pass;

}

DLL_EXPORT int test_vcpu_instr_asl(ITesting *t) {
    uint8_t program[]= {
        // 0xE6 - opcode, ASL
        // 0x00 - OP Size, 0 = byte
        // 0x03 - DstRegMode, RRRR|MMMM = 0000 | 0011 => d0 | 3 => 3 = register
        // 0x01 - SrcRegMode, 0000|MMMM = 0000 | 0001 => xx | 1 => Immediate
        // 0x01 - Immediate value (0x01)

        0xE6,0x00,0x03,0x01,0x01    // lsl.b d0, #1
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();

    vcpu.QuickStart(program, 1024);
    regs.dataRegisters[0].data.byte = 0x20;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);

    TR_ASSERT(t, regs.dataRegisters[0].data.byte == 0x40);

    return kTR_Pass;

}

DLL_EXPORT int test_vcpu_instr_cmp(ITesting *t) {
    uint8_t program[]={
        0x90,0x00,0x03,0x01,0x33,       // cmp.b d0, 0x33
        0x90,0x00,0x03,0x01,0x33,       // cmp.b d0, 0x33
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();
    auto &status = vcpu.GetStatusReg();

    vcpu.QuickStart(program, 1024);

    // Set d0 = 0x20 and execute
    regs.dataRegisters[0].data.byte = 0x20;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);
    TR_ASSERT(t, status.flags.zero == 0);
    TR_ASSERT(t, status.flags.negative == 1);

    // Set d0 = 0x33 and execute same again
    regs.dataRegisters[0].data.byte = 0x33;
    res = vcpu.Step();
    TR_ASSERT(t, res);
    TR_ASSERT(t, status.flags.zero == 1);
    TR_ASSERT(t, status.flags.negative == 0);

    return kTR_Pass;
}
DLL_EXPORT int test_vcpu_instr_cmp_absolute(ITesting *t) {


    uint8_t program[]={
            // op code = 0x90 => cmp
            // op size = 0x03 => long
            // dst reg mode = 0x02 => absolute
            // [value]
            // src reg mode = 0x01 => immediate
            // [value]
            //
            0x90, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,   // cmp.l (0x00000015),0x01
            0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // this should be addres 0x15
            // 0     1     2     3     4     5     6     7     8     9     10   11   12    13    14    15    16    17    18   19
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();
    auto &status = vcpu.GetStatusReg();

    vcpu.QuickStart(program, 1024);

    // Set d0 = 0x20 and execute
    auto res = vcpu.Step();
    fmt::println("disasm: {}", vcpu.GetLastDecodedInstr()->ToString());
    TR_ASSERT(t, res);
    TR_ASSERT(t, status.flags.zero == 0);
    TR_ASSERT(t, status.flags.negative == 1);

    return kTR_Pass;
}


DLL_EXPORT int test_vcpu_instr_beq(ITesting *t) {
    uint8_t program[]={
        0x90,0x00,0x03,0x01,0x33,       // cmp.b d0, 0x33
        0xd0,0x00,0x01, 0xf7,            // beq.b -10
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();
    auto &status = vcpu.GetStatusReg();

    vcpu.QuickStart(program, 1024);

    // Set d0 = 0x20 and execute
    auto instrPtrLoopPoint = vcpu.GetInstrPtr().data.longword;
    regs.dataRegisters[0].data.byte = 0x33;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);
    TR_ASSERT(t, status.flags.zero == 1);

    vcpu.Step();
    TR_ASSERT(t, res);
    TR_ASSERT(t, vcpu.GetInstrPtr().data.longword == instrPtrLoopPoint);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_instr_bne(ITesting *t) {
    uint8_t program[]={
        0x90,0x00,0x03,0x01,0x33,       // cmp.b d0, 0x33
        0xd1,0x00,0x01,0xf7,            // bne.b -9
    };
    VirtualCPU vcpu;
    auto &regs = vcpu.GetRegisters();
    auto &status = vcpu.GetStatusReg();

    vcpu.QuickStart(program, 1024);

    // Set d0 = 0x20 and execute
    auto instrPtrLoopPoint = vcpu.GetInstrPtr().data.longword;
    regs.dataRegisters[0].data.byte = 0x32;
    auto res = vcpu.Step();
    TR_ASSERT(t, res);
    TR_ASSERT(t, status.flags.zero == 0);

    vcpu.Step();
    TR_ASSERT(t, res);
    TR_ASSERT(t, vcpu.GetInstrPtr().data.longword == instrPtrLoopPoint);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_halt(ITesting *t) {
    uint8_t program[]={
        0xf1,0x00,0x00,0x00,   // nop
        0x00,   0x00,0x00,0x00,// break
        0xf1, 0x00,0x00,0x00,  // nop - should never execute
        0xf1,  0x00,0x00,0x00, // nop
        0xf1,  0x00,0x00,0x00, // nop
        0xf1,  0x00,0x00,0x00, // nop
    };
    VirtualCPU vcpu;
    auto &status = vcpu.GetStatusReg();
    vcpu.QuickStart(program, 1024);
    TR_ASSERT(t, status.flags.halt == 0);
    TR_ASSERT(t, vcpu.Step());
    TR_ASSERT(t, vcpu.Step());
    auto instrPtr = vcpu.GetInstrPtr();
    TR_ASSERT(t, status.flags.halt == 1);
    TR_ASSERT(t, vcpu.Step());
    TR_ASSERT(t, instrPtr.data.longword == vcpu.GetInstrPtr().data.longword);

    return kTR_Pass;
}

DLL_EXPORT int test_vcpu_disasm(ITesting *t) {
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
        0x20,0x01,0x13,0x03,    // move.w d1, d0
        0x20,0x01,0x53,0x13,    // move.w d5, d1
        0x20,0x00,0x23,0x53,    // move.b d2, d5
        0x00,0x00,0x00,0x00,
    };

    VirtualCPU vcpu;
    vcpu.QuickStart(program, 1024);
    auto &regs = vcpu.GetRegisters();
    while(!vcpu.IsHalted()) {
        vcpu.Step();
        auto lastInstr = vcpu.GetLastDecodedInstr();
        auto str = lastInstr->ToString();
        fmt::println("{}", str);
        if (vcpu.IsHalted()) break;
    }

    return kTR_Pass;
}


