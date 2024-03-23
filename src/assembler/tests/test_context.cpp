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
DLL_EXPORT int test_context_multiunit_export_use(ITesting *t);
DLL_EXPORT int test_context_multiunit_export_simple(ITesting *t);
DLL_EXPORT int test_context_multiunit_export_implicit(ITesting *t);
DLL_EXPORT int test_context_multiunit_export_unknown(ITesting *t);
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

//
// Using an exported variable
//
DLL_EXPORT int test_context_multiunit_export_use(ITesting *t) {
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

//
// Simple export. First unit exports a symbol, second unit calls the symbol
//
DLL_EXPORT int test_context_multiunit_export_simple(ITesting *t) {
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

//
// Test of unknown symbol - in one unit we export something and in the other unit we use something else
// this leads to a linker error
//
DLL_EXPORT int test_context_multiunit_export_unknown(ITesting *t) {
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
                    "call no_such_func\n"\
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
    // This shouldn't link - as there is no export for 'no_such_func'
    TR_ASSERT(t, !compiler.Link());

    return kTR_Pass;

}

//
// Implicit exports. We are calling before the export has been declared.
// The compiler will see that the symbol (mymain) is not in the export list nor in the local identifier list
// it will then add an import..  When merging the units the import/export list is also merged (and updated) and when
// the fully merged code hits the linker it resolves it properly...
//
DLL_EXPORT int test_context_multiunit_export_implicit(ITesting *t) {
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
