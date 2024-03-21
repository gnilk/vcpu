//
// Created by gnilk on 10.03.2024.
//
#include <stdio.h>
#include <filesystem>

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Compiler/Compiler.h"
#include "Compiler/CompiledUnit.h"
#include "HexDump.h"
#include "Compiler/StmtEmitter.h"

using namespace gnilk::assembler;

extern "C" {
DLL_EXPORT int test_stmtemitter(ITesting *t);
DLL_EXPORT int test_stmtemitter_data_byte(ITesting *t);
DLL_EXPORT int test_stmtemitter_call_label(ITesting *t);
DLL_EXPORT int test_stmtemitter_call_relative_label(ITesting *t);

}

DLL_EXPORT int test_stmtemitter(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_stmtemitter_data_byte(ITesting *t) {
    std::vector<uint8_t> expectedBinary = {
            0,1,2,3,
    };
    std::vector<std::string> code={
            {"data: dc.b 0,1,2,3"}
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast != nullptr);

    ast->Dump();
    Context context;

    std::vector<EmitStatementBase::Ref> emitters;

    for(auto &stmt : ast->Body()) {
        auto emitter = EmitStatementBase::Create(stmt);
        TR_ASSERT(t, emitter != nullptr);
        emitter->Process(context.CurrentUnit());
        emitters.push_back(emitter);
    }


    return kTR_Pass;
}

DLL_EXPORT int test_stmtemitter_call_label(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
            0xc0,0x03,0x01,                             // 0, Call label, opSize = lword, [reg|mode] = 0|immediate, <address of label> = 0x0d
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0d,    // 3, This is the replace point - sits at offset 4
            0xf1,                       // 11
            0x00,                       // 12 WHALT!
            0xf1,                       // 13 <- call should go here (offset of label)
            0xf1,                       // 14
            0xf0,                       // 15 <- return, should be ip+1 => 5
    };

    std::vector<std::string> codes={
            {
                    "call    label\n "\
        "nop      \n"\
        "brk        \n"\
        "label: \n"\
        "nop \n"\
        "nop \n"\
        "ret"
            }
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(codes[0]);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res == true);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}

DLL_EXPORT int test_stmtemitter_call_relative_label(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
            0xc0,0x00,0x01,0x03,        // 0, Call IP+2   ; from en of instr -> 4+3 => 7
            0xf1,                       // 4
            0x00,                       // 5 WHALT!
            0xf1,                       // 6 <- call should go here
            0xf1,                       // 7
            0xf0,                       // 8 <- return, should be ip+1 => 5
    };
    std::vector<std::string> codes={
            {
                    "call.b    0x03\n "\
        "nop      \n"\
        "brk        \n"\
        "label:\n"\
        "nop \n"\
        "nop \n"\
        "ret"
            }
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(codes[0]);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}
