//
// Created by gnilk on 12.05.24.
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
    DLL_EXPORT int test_exceptions(ITesting *t);
    DLL_EXPORT int test_exceptions_illegal_instr(ITesting *t);
    DLL_EXPORT int test_exceptions_in_isr(ITesting *t);
}
DLL_EXPORT int test_exceptions(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_exceptions_illegal_instr(ITesting *t) {
    VirtualCPU vcpu;
    // See: 'Interrupt.h' for definition
    ISR_VECTOR_TABLE isrTable = {
            .exp_illegal_instr = 0x1000,
    };

    uint8_t expRoutine[]={
            // move.l d0,0x01
            0x20,0x03,0x03,0x01, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
            // syscall
            OperandCode::SYS,
            // nop
            OperandCode::NOP,
            // rti
            OperandCode::RTE,
    };
    uint8_t mainCode[]={
            // nop,nop,nop,brk
            0x01,   // This OP-Code is not defined - we should probably have a specific to raise the exception
            OperandCode::NOP, OperandCode::NOP, OperandCode::NOP, OperandCode::BRK
    };
    vcpu.Begin(ram, 32*4096);
    vcpu.LoadDataToRam(0, &isrTable, sizeof(isrTable));
    vcpu.LoadDataToRam(0x1000, expRoutine, sizeof(expRoutine));
    vcpu.LoadDataToRam(0x2000, mainCode, sizeof(mainCode));

    int exp_counter = 0;
    vcpu.RegisterSysCall(0x01, "count",[&exp_counter](Registers &regs, CPUBase *cpu) {
        exp_counter++;
        fmt::println("'count' called from exception handler, counter={}",exp_counter);
    });

    vcpu.SetInstrPtr(0x2000);
    vcpu.EnableException(CPUExceptionFlag::InvalidInstruction);
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

    fmt::println("exp counter={}", exp_counter);
    TR_ASSERT(t, exp_counter == 1);

    return kTR_Pass;
}

// Test how exception from within an ISR behaves
DLL_EXPORT int test_exceptions_in_isr(ITesting *t) {
    VirtualCPU vcpu;
    ISR_VECTOR_TABLE isrTable = {
            .exp_illegal_instr = 0x1200,
            .isr0 = 0x1000,
    };

    uint8_t isrRoutine[]={
            // move.l d0,0x01
            0x20,0x03,0x03,0x01, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,   // 0
            // syscall
            0xc2,                                                           // 12
            // nop
            OperandCode::NOP,                                               // 13
            0x01,   // Illegal                                              // 14
            // rti
            OperandCode::RTI,                                               // 15
    };

    uint8_t expRoutine[]={
            // move.l d0,0x01
            0x20,0x03,0x03,0x01, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
            // syscall
            OperandCode::SYS,
            // nop
            OperandCode::NOP,
            // rte
            OperandCode::RTE,
    };

    uint8_t mainCode[]={
            // nop,nop,nop,brk
            OperandCode::NOP, OperandCode::NOP, OperandCode::NOP, OperandCode::BRK
    };
    vcpu.Begin(ram, 32*4096);
    vcpu.LoadDataToRam(0, &isrTable, sizeof(isrTable));
    vcpu.LoadDataToRam(0x1000, isrRoutine, sizeof(isrRoutine));
    vcpu.LoadDataToRam(0x1200, expRoutine, sizeof(expRoutine));
    vcpu.LoadDataToRam(0x2000, mainCode, sizeof(mainCode));

    int irq_counter = 0;
    int exp_counter = 0;
    vcpu.RegisterSysCall(0x01, "count",[&irq_counter](Registers &regs, CPUBase *cpu) {
        irq_counter++;
        fmt::println("ISR - count, counter={}",irq_counter);
    });
    vcpu.RegisterSysCall(0x02, "exp_handler",[&exp_counter](Registers &regs, CPUBase *cpu) {
        exp_counter++;
        fmt::println("EXP - count, counter={}",exp_counter);
    });

    vcpu.SetInstrPtr(0x2000);
    vcpu.EnableInterrupt(INT0);
    vcpu.EnableException(CPUExceptionFlag::InvalidInstruction);
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

    fmt::println("irq counter={}", irq_counter);
    fmt::println("irq counter={}", exp_counter);

    return kTR_Pass;
}

