//
// Created by gnilk on 14.12.23.
//
#include <testinterface.h>
#include "Parser/Parser.h"

using namespace gnilk::assembler;

extern "C" {
    DLL_EXPORT int test_parser(ITesting *t);
    DLL_EXPORT int test_parser_instruction(ITesting *t);
    DLL_EXPORT int test_parser_declaration(ITesting *t);
}
DLL_EXPORT int test_parser(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_parser_instruction(ITesting *t) {
    Parser parser;

    auto ast = parser.ProduceAST("move.l d0,0x45");
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}

DLL_EXPORT int test_parser_declaration(ITesting *t) {
    Parser parser;

    auto ast = parser.ProduceAST("data: dc.b 1,2,3,4");
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}
