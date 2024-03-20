//
// Created by gnilk on 20.03.24.
//
#include <stdio.h>
#include <filesystem>

#include <testinterface.h>

#include "Parser/Parser.h"

#include "Compiler/Compiler.h"
#include "Compiler/Context.h"
#include "Compiler/CompiledUnit.h"
#include "HexDump.h"

using namespace gnilk::assembler;

extern "C" {
DLL_EXPORT int test_context(ITesting *t);
DLL_EXPORT int test_context_multiunit_simple(ITesting *t);
DLL_EXPORT int test_context_multiunit_orgstmt(ITesting *t);
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
