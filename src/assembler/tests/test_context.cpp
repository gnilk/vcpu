//
// Created by gnilk on 20.03.24.
//
#include <stdio.h>
#include <filesystem>

#include <testinterface.h>

#include "Parser/Parser.h"

#include "Compiler/Compiler.h"
#include "Compiler/Context.h"
#include "Compiler/CompileUnit.h"
#include "HexDump.h"

using namespace gnilk::assembler;

extern "C" {
DLL_EXPORT int test_context(ITesting *t);
DLL_EXPORT int test_context_multiunit_simple(ITesting *t);
DLL_EXPORT int test_context_multiunit_orgstmt(ITesting *t);
DLL_EXPORT int test_context_multiunit_useexport(ITesting *t);
DLL_EXPORT int test_context_multiunit_callunit(ITesting *t);
DLL_EXPORT int test_context_multiunit_dualcall(ITesting *t);
}

DLL_EXPORT int test_context(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_context_multiunit_simple(ITesting *t) {
    static std::string code[] = {
            {
                    ".code\n" \
                "move.l d0, 0x4711\n",
            },
            {
                    ".code\n" \
                "move.l d1, 0x4712\n",
            },

    };

    Compiler compiler;
    Parser parser;

    auto ast1 = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast1 != nullptr);
    auto ast2 = parser.ProduceAST(code[1]);
    TR_ASSERT(t, ast2 != nullptr);
    TR_ASSERT(t, compiler.Compile(ast1));
    TR_ASSERT(t, compiler.Compile(ast2));
    TR_ASSERT(t, compiler.Link());

    auto &data = compiler.Data();
    TR_ASSERT(t, !data.empty());

    return kTR_Pass;
}

DLL_EXPORT int test_context_multiunit_orgstmt(ITesting *t) {
    static std::string code[] = {
            {
                    ".code\n" \
                    ".org 0x100\n" \
                    "move.l d0, 0x4711\n",
            },
            {
                    ".code\n" \
                    "move.l d1, 0x4712\n",
                },

    };

    Compiler compiler;
    Parser parser;

    // If code 1 is compiled first the write address will be at address 0x100+size of code for code 2
    auto ast1 = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast1 != nullptr);
    // Here we just reuse the .code segment and append code at the current write point...
    auto ast2 = parser.ProduceAST(code[1]);
    TR_ASSERT(t, ast2 != nullptr);
    TR_ASSERT(t, compiler.Compile(ast1));
    TR_ASSERT(t, compiler.Compile(ast2));
    TR_ASSERT(t, compiler.Link());

    auto &data = compiler.Data();
    TR_ASSERT(t, !data.empty());
    // We should have data above address 0x100
    TR_ASSERT(t, data.size() > 0x100);

    return kTR_Pass;
}
DLL_EXPORT int test_context_multiunit_useexport(ITesting *t) {
    static std::string code[] = {
            {
                    ".code\n" \
                    "export myvar\n"\
                    "move.l d1, 0x4711\n"\
                    "myvar:  dc.l 0x4712\n"
                    },
            {
                    ".code\n" \
                    "move.l d1, myvar",
            },

    };

    Compiler compiler;
    Parser parser;

    auto ast1 = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast1 != nullptr);
    // Here we just reuse the .code segment and append code at the current write point...
    auto ast2 = parser.ProduceAST(code[1]);
    TR_ASSERT(t, ast2 != nullptr);
    TR_ASSERT(t, compiler.Compile(ast1));
    TR_ASSERT(t, compiler.Compile(ast2));
    TR_ASSERT(t, compiler.Link());

    auto &data = compiler.Data();
    TR_ASSERT(t, !data.empty());

    return kTR_Pass;

}

DLL_EXPORT int test_context_multiunit_callunit(ITesting *t) {
    static std::string code[] = {
            {
                    ".code\n" \
                    "export myfunc\n"\
                    "move.l d1, 0x4711\n"\
                    ".org 0x100\n"\
                    "myfunc:\n"\
                    "move.l d0, 0x4712\n"\
                    "ret\n"\
            },
            {
                    ".code\n" \
                    ".org 0x200\n"\
                    "call myfunc\n"\
                    "ret\n"
            },

    };

    Compiler compiler;
    Parser parser;

    auto ast1 = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast1 != nullptr);
    // Here we just reuse the .code segment and append code at the current write point...
    auto ast2 = parser.ProduceAST(code[1]);
    TR_ASSERT(t, ast2 != nullptr);
    TR_ASSERT(t, compiler.Compile(ast1));
    TR_ASSERT(t, compiler.Compile(ast2));
    TR_ASSERT(t, compiler.Link());

    auto &data = compiler.Data();
    TR_ASSERT(t, !data.empty());
    TR_ASSERT(t, data.size() > 200);

    printf("Binary Size: %d\n", (int)data.size());
    HexDump::ToConsole(data.data()+0, 16);
    HexDump::ToConsole(data.data()+0x100, 16);
    HexDump::ToConsole(data.data()+0x200, 16);

    return kTR_Pass;

}

DLL_EXPORT int test_context_multiunit_dualcall(ITesting *t) {
    static std::string code[] = {
            {
                    ".code\n" \
                    "export myfunc\n"\
                    "call mymain\n"\
                    ".org 0x100\n"\
                    "myfunc:\n"\
                    "move.l d0, 0x4712\n"\
                    "ret\n"\
            },
            {
                    ".code\n" \
                    ".org 0x200\n"\
                    "export mymain\n"\
                    "mymain:\n"\
                    "call myfunc\n"\
                    "ret\n"
            },

    };

    Compiler compiler;
    Parser parser;

    auto ast1 = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast1 != nullptr);
    // Here we just reuse the .code segment and append code at the current write point...
    auto ast2 = parser.ProduceAST(code[1]);
    TR_ASSERT(t, ast2 != nullptr);
    ast1->Dump();
    ast2->Dump();
    TR_ASSERT(t, compiler.Compile(ast1));
    TR_ASSERT(t, compiler.Compile(ast2));
    TR_ASSERT(t, compiler.Link());

    auto &data = compiler.Data();
    TR_ASSERT(t, !data.empty());
    TR_ASSERT(t, data.size() > 200);

    printf("Binary Size: %d\n", (int)data.size());
    HexDump::ToConsole(data.data()+0, 16);
    HexDump::ToConsole(data.data()+0x100, 16);
    HexDump::ToConsole(data.data()+0x200, 16);

    return kTR_Pass;

}
