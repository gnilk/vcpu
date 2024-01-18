//
// Created by gnilk on 16.01.2024.
//

#include <stdint.h>
#include <vector>
#include <testinterface.h>
#include <thread>
#include <chrono>

#include "VirtualCPU.h"
#include "Timer.h"

using namespace gnilk;
using namespace gnilk::vcpu;

static uint8_t ram[32*4096]; // 32 pages more than enough for this test

static void DumpStatus(const VirtualCPU &cpu);
static void DumpRegs(const VirtualCPU &cpu);

extern "C" {
    DLL_EXPORT int test_int(ITesting *t);
    DLL_EXPORT int test_int_invoke(ITesting *t);
}
DLL_EXPORT int test_int(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_int_invoke(ITesting *t) {
    VirtualCPU vcpu;
    uint64_t isrTable[]= {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x1000,
    };
    uint8_t isrRoutine[]={
        // move.l d0,0x01
        0x20,0x03,0x03,0x01, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
        // syscall
        0xc2,
        // nop
        0xf1,
        // rti
        0xf2,
    };
    uint8_t mainCode[]={
        // nop,nop,nop,brk
        0xf1, 0xf1, 0xf1, 0x00
    };
    vcpu.Begin(ram, 32*4096);
    vcpu.LoadDataToRam(0, isrTable, sizeof(isrTable));
    vcpu.LoadDataToRam(0x1000, isrRoutine, sizeof(isrTable));
    vcpu.LoadDataToRam(0x2000, mainCode, sizeof(isrTable));

    int irq_counter = 0;
    vcpu.RegisterSysCall(0x01, "count",[&irq_counter](Registers &regs, CPUBase *cpu) {
        irq_counter++;
        fmt::println("count, counter={}",irq_counter);
    });

    vcpu.SetInstrPtr(0x2000);
    for(int i=0;i<15;i++) {
        vcpu.Step();
        // this print is quite wrong..
        if ((vcpu.GetISRState() == CPUISRState::Executing) || (!vcpu.GetStatusReg().flags.halt)) {
            fmt::println("{} {}{:#x} {}",i,
                (vcpu.GetLastDecodedInstr()->GetISRStateBefore()==CPUISRState::Executing)?'*':' ',
                vcpu.GetLastDecodedInstr()->cpuRegistersBefore.instrPointer.data.longword,
                vcpu.GetLastDecodedInstr()->ToString());
        } else {
            fmt::println("{} --- halted ---", i);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(250));
    }

    TR_ASSERT(t, irq_counter == 3);

    return kTR_Pass;
}




/// helpers
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
