//
// Created by gnilk on 07.05.24.
//
#include <stdio.h>
#include <filesystem>

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Compiler/Compiler.h"
#include "Compiler/CompileUnit.h"
#include "VirtualCPU.h"
#include "HexDump.h"

using namespace gnilk::assembler;

static uint8_t ram[512*1024] = {};

extern "C" {
    DLL_EXPORT int test_linker(ITesting *t);
    DLL_EXPORT int test_linker_simple(ITesting *t);
    DLL_EXPORT int test_linker_twoseg(ITesting *t);
    DLL_EXPORT int test_linker_multiorg(ITesting *t);
    DLL_EXPORT int test_linker_relocate_sameseg(ITesting *t);
    DLL_EXPORT int test_linker_relocate_otherseg(ITesting *t);
    DLL_EXPORT int test_linker_relocate_otherseg_withorg(ITesting *t);
}

DLL_EXPORT int test_linker(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_linker_simple(ITesting *t) {
    const char code[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "   dc.b 0xaa,0xbb,0xcc,0xdd\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &data = compiler.Data();
    TR_ASSERT(t, data[0] == 0xaa);
    TR_ASSERT(t, data[1] == 0xbb);
    TR_ASSERT(t, data[2] == 0xcc);
    TR_ASSERT(t, data[3] == 0xdd);

    return kTR_Pass;
}

DLL_EXPORT int test_linker_twoseg(ITesting *t) {
    // Two segements directly after each-other...
    const char code[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "   dc.b 0xaa,0xbb,0xcc,0xdd\n"\
        "  .data \n"\
        "   dc.b 0x11,0x22,0x33,0x44\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &data = compiler.Data();
    TR_ASSERT(t, data[0] == 0xaa);
    TR_ASSERT(t, data[1] == 0xbb);
    TR_ASSERT(t, data[2] == 0xcc);
    TR_ASSERT(t, data[3] == 0xdd);

    TR_ASSERT(t, data[4] == 0x11);
    TR_ASSERT(t, data[5] == 0x22);
    TR_ASSERT(t, data[6] == 0x33);
    TR_ASSERT(t, data[7] == 0x44);

    return kTR_Pass;
}
DLL_EXPORT int test_linker_multiorg(ITesting *t) {
    const char code[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "   dc.b 0x11,0x22,0x33,0x44\n"\
        "   .org 100\n"\
        "   dc.b 0x55,0x66,0x77,0x88\n"\
        "   .org 200\n"\
        "   dc.b 0x99,0xaa,0xbb,0xcc\n"\
        "   ret\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &data = compiler.Data();
    TR_ASSERT(t, data[0] == 0x11);
    TR_ASSERT(t, data[1] == 0x22);
    TR_ASSERT(t, data[2] == 0x33);
    TR_ASSERT(t, data[3] == 0x44);

    TR_ASSERT(t, data[100] == 0x55);
    TR_ASSERT(t, data[101] == 0x66);
    TR_ASSERT(t, data[102] == 0x77);
    TR_ASSERT(t, data[103] == 0x88);

    TR_ASSERT(t, data[200] == 0x99);
    TR_ASSERT(t, data[201] == 0xaa);
    TR_ASSERT(t, data[202] == 0xbb);
    TR_ASSERT(t, data[203] == 0xcc);

    return kTR_Pass;
}

DLL_EXPORT int test_linker_relocate_sameseg(ITesting *t) {
    const char code[]= {
            "  .code \n"\
        "   .org 0x0000 \n"\
        "   lea d0,label\n"\
        "   brk\n"\
        " label:\n"\
        "   dc.b 0x99,0xaa,0xbb,0xcc\n"\
        "   ret\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &binary = compiler.Data();
    printf("Binary:\n");
    HexDump::ToConsole(binary.data(),binary.size());


    memcpy(ram, binary.data(), binary.size());

    printf("Disasm:\n");
    gnilk::vcpu::VirtualCPU cpu;
    cpu.QuickStart(ram, 1024*512);
    // Disassemble a couple of instructions to get the feel...`
    for(int i=0;i<8;i++) {
        auto instrPtr = cpu.GetRegisters().instrPointer.data.dword;
        cpu.Step();
        fmt::println("{}\t{}", instrPtr, cpu.GetLastDecodedInstr()->ToString());
        if (i == 0) {
            auto addr = cpu.GetRegisters().dataRegisters[0].data.longword;
            TR_ASSERT(t, addr == 0x0d);
        }
        if (cpu.IsHalted()) {
            break;
        }
    }

    return kTR_Pass;
}
DLL_EXPORT int test_linker_relocate_otherseg(ITesting *t) {
    const char code[]= {
            "  .code \n"\
        "   .org 0x0000 \n"\
        "   lea d0,label\n"\
        "   brk\n"\
        "  .data \n"\
        " label:\n"\
        "   dc.b 0x99,0xaa,0xbb,0xcc\n"\
        "   ret\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &binary = compiler.Data();
    printf("Binary:\n");
    HexDump::ToConsole(binary.data(),binary.size());


    memcpy(ram, binary.data(), binary.size());

    printf("Disasm:\n");
    gnilk::vcpu::VirtualCPU cpu;
    cpu.QuickStart(ram, 1024*512);
    // Disassemble a couple of instructions to get the feel...`
    for(int i=0;i<8;i++) {
        auto instrPtr = cpu.GetRegisters().instrPointer.data.dword;
        cpu.Step();
        fmt::println("{}\t{}", instrPtr, cpu.GetLastDecodedInstr()->ToString());
        if (i == 0) {
            auto addr = cpu.GetRegisters().dataRegisters[0].data.longword;
            TR_ASSERT(t, addr == 0x0d);
        }
        if (cpu.IsHalted()) {
            break;
        }
    }

    return kTR_Pass;

}

DLL_EXPORT int test_linker_relocate_otherseg_withorg(ITesting *t) {
    const char code[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "   lea d0,label\n"\
        "   brk\n"\
        "  .data \n"\
        "  .org 0x100\n"\
        "   dc.l 0\n"\
        " label:\n"\
        // should be at address 0x108
        "   dc.b 0x99,0xaa,0xbb,0xcc\n"\
        "   ret\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &binary = compiler.Data();
    printf("Binary:\n");
    HexDump::ToConsole(binary.data(),binary.size());


    memcpy(ram, binary.data(), binary.size());

    printf("Disasm:\n");
    gnilk::vcpu::VirtualCPU cpu;
    cpu.QuickStart(ram, 1024*512);
    // Disassemble a couple of instructions to get the feel...`
    for(int i=0;i<8;i++) {
        auto instrPtr = cpu.GetRegisters().instrPointer.data.dword;
        cpu.Step();
        fmt::println("{}\t{}", instrPtr, cpu.GetLastDecodedInstr()->ToString());
        if (i == 0) {
            auto addr = cpu.GetRegisters().dataRegisters[0].data.longword;
            TR_ASSERT(t, addr == 0x0108);
        }
        if (cpu.IsHalted()) {
            break;
        }
    }

    return kTR_Pass;

}


