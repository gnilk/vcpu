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
        0xf1, 0xf2, 0xf1, 0xf1, 0xf1
    };
    uint8_t mainCode[]={
        0xf1, 0xf1, 0xf1, 0x00
    };
    vcpu.Begin(ram, 32*4096);
    vcpu.LoadDataToRam(0, isrTable, sizeof(isrTable));
    vcpu.LoadDataToRam(0x1000, isrRoutine, sizeof(isrTable));
    vcpu.LoadDataToRam(0x2000, mainCode, sizeof(isrTable));

    vcpu.SetInstrPtr(0x2000);
    for(int i=0;i<10;i++) {
        vcpu.Step();
        fmt::println("{:#x} {}",vcpu.GetLastDecodedInstr()->GetInstrStartOfs(), vcpu.GetLastDecodedInstr()->ToString());
        std::this_thread::sleep_for(std::chrono::microseconds(250));
    }

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
