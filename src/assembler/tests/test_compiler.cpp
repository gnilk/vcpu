//
// Created by gnilk on 15.12.23.
//

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Compiler/Compiler.h"

using namespace gnilk::assembler;

extern "C" {
    DLL_EXPORT int test_compiler(ITesting *t);
    DLL_EXPORT int test_compiler_move_immediate(ITesting *t);
    DLL_EXPORT int test_compiler_move_reg2reg(ITesting *t);
}

DLL_EXPORT int test_compiler(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_compiler_move_immediate(ITesting *t) {
    Parser parser;
    Compiler compiler;

    // move.b d0, 0x45
    static std::vector<uint8_t> expectedCase1 = {0x20,0x00,0x03,0x01, 0x45};
    auto prg = parser.ProduceAST("move.b d0,0x45");
    auto res = compiler.GenerateCode(prg);
    TR_ASSERT(t, res);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase1);

    // move.w d0, 0x4433
    static std::vector<uint8_t> expectedCase2 = {0x20,0x01,0x03,0x01, 0x44,0x33};
    prg = parser.ProduceAST("move.w d0, 0x4433");
    TR_ASSERT(t, compiler.GenerateCode(prg));
    binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase2);

    // move.d d0, 0x44332211
    static std::vector<uint8_t> expectedCase3 = {0x20,0x02,0x03,0x01, 0x44,0x33,0x22,0x11};
    prg = parser.ProduceAST("move.d d0, 0x44332211");
    TR_ASSERT(t, compiler.GenerateCode(prg));
    binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase3);

    // LWORD currently not supported by AST tree...

    // Different register

    // move.b d2, 0x44
    static std::vector<uint8_t> expectedCase4 = {0x20,0x00,0x23,0x01, 0x44};
    prg = parser.ProduceAST("move.b d2, 0x44");
    TR_ASSERT(t, compiler.GenerateCode(prg));
    binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase4);

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_move_reg2reg(ITesting *t) {
    std::vector<uint8_t> binaries[]= {
        {0x20,0x01,0x13,0x03},    // move.w d1, d0
        {0x20,0x01,0x53,0x13},    // move.w d5, d1
        {0x20,0x00,0x23,0x53},    // move.b d2, d5
    };
    std::vector<std::string> code={
        {"move.w d1, d0"},
        {"move.w d5, d1"},
        {"move.b d2, d5"},
    };

    Parser parser;
    Compiler compiler;

    for(int i=0;i<code.size();i++) {
        auto ast = parser.ProduceAST(code[i]);
        TR_ASSERT(t, ast != nullptr);
        TR_ASSERT(t, compiler.GenerateCode(ast));
        auto binary = compiler.Data();
        TR_ASSERT(t, binary == binaries[i]);
    }

    return kTR_Pass;
}
