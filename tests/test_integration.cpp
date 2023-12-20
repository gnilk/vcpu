//
// Created by gnilk on 19.12.23.
//
// These tests will test everything from parser, compiler, linker, emulator in one go..
//
#include <stdint.h>
#include <vector>
#include <filesystem>
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
    DLL_EXPORT int test_integration_file(ITesting *t);
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
    "syscall\n"\
    "brk\n"\
    "";

    TR_ASSERT(t, RunAndExecute(code, vcpu));
    TR_ASSERT(t, wasCalled);

    return kTR_Pass;
}

DLL_EXPORT int test_integration_file(ITesting *t) {

    std::filesystem::path pathToSrcFile("Assets/test.asm");
    size_t szFile = file_size(pathToSrcFile);
    char *data = (char *)malloc(szFile + 10);
    TR_ASSERT(t, data != nullptr);

    memset(data, 0, szFile + 10);


    FILE *f = fopen("Assets/test.asm", "r+");
    TR_ASSERT(t, f != nullptr);
    auto nRead = fread(data, 1, szFile, f);
    fclose(f);

    VirtualCPU vcpu;

    bool wasCalled = false;
    vcpu.RegisterSysCall(0x01, "writeline",[&wasCalled](Registers &regs, CPUBase *cpu) {
       fmt::println("wefwef - from syscall");
        wasCalled = true;
    });

    TR_ASSERT(t, RunAndExecute(data, vcpu));

    return kTR_Pass;
}