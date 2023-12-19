//
// Created by gnilk on 19.12.23.
//
// These tests will test everything from parser, compiler, linker, emulator in one go..
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "Compiler/Compiler.h"
#include "Parser/Parser.h"

using namespace gnilk;
using namespace gnilk::vcpu;

static void DumpStatus(const VirtualCPU &cpu);
static void DumpRegs(const VirtualCPU &cpu);


extern "C" {
    DLL_EXPORT int test_integration(ITesting *t);
    DLL_EXPORT int test_integration_syscall(ITesting *t);
}
DLL_EXPORT int test_integration(ITesting *t) {
    return kTR_Pass;
}

static uint8_t ram[512*1024] = {};

//
// Compile, Link and Execute code in the Emulator - supply the code and the cpu (inkl. any configuration - except RAM)
//
static bool RunAndExecute(const char *code, VirtualCPU &cpu) {
    assembler::Parser parser;
    assembler::Compiler compiler;
    auto ast = parser.ProduceAST(code);
    if (ast == nullptr) {
        return false;
    }
    if (!compiler.CompileAndLink(ast)) {
        return false;
    }
    auto firmware = compiler.Data();

    memcpy(ram, firmware.data(), firmware.size());

    cpu.Begin(ram, 1024*512);
    while(cpu.Step()) {

    }
    return true;

}

DLL_EXPORT int test_integration_syscall(ITesting *t) {
    VirtualCPU vcpu;

    bool wasCalled = false;
    vcpu.RegisterSysCall(0x01, "writeline",[&wasCalled](Registers &regs, CPUBase *cpu) {
       fmt::println("wefwef - from syscall");
        wasCalled = true;
    });

    const char code[]=".code\n"\
    "move.l d0, 0x01\n"\
    "sys\n"\
    "brk\n"\
    "";

    TR_ASSERT(t, RunAndExecute(code, vcpu));
    TR_ASSERT(t, wasCalled);

    return kTR_Pass;
}

