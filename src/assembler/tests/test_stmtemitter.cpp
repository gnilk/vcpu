//
// Created by gnilk on 10.03.2024.
//
#include <stdio.h>
#include <filesystem>

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Compiler/Compiler.h"
#include "Compiler/CompileUnit.h"
#include "HexDump.h"
#include "Compiler/StmtEmitter.h"

using namespace gnilk::assembler;
using namespace gnilk::vcpu;

extern "C" {
DLL_EXPORT int test_stmtemitter(ITesting *t);
DLL_EXPORT int test_stmtemitter_data_byte(ITesting *t);
DLL_EXPORT int test_stmtemitter_struct(ITesting *t);
DLL_EXPORT int test_stmtemitter_struct_in_struct(ITesting *t);
DLL_EXPORT int test_stmtemitter_structref(ITesting *t);
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
    gnilk::assembler::Context context;

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
            OperandCode ::NOP,                       // 11
            OperandCode ::BRK,                       // 12 WHALT!
            OperandCode ::NOP,                       // 13 <- call should go here (offset of label)
            OperandCode ::NOP,                       // 14
            OperandCode ::RET,                       // 15 <- return, should be ip+1 => 5
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
            OperandCode ::NOP,                       // 4
            OperandCode ::BRK,                       // 5 WHALT!
            OperandCode ::NOP,                       // 6 <- call should go here
            OperandCode ::NOP,                       // 7
            OperandCode ::RET,                       // 8 <- return, should be ip+1 => 5
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


DLL_EXPORT int test_stmtemitter_struct(ITesting *t) {
    // Should generate 16 bytes
    // 1+2+4+8 = 15
    const char srcCode[]= {
            "struct table {\n"\
        "   some_byte dc.b 1\n"\
        "   some_word dc.w 1\n"\
        "   some_dword dc.d 1\n"\
        "   some_long dc.l 1\n"\
        "}\n"\
        "data: dc.struct table {}"
        ""
    };

    Parser parser;
    Compiler compiler;

    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
//    TR_ASSERT(t, binary == expectedBinary);

    TR_ASSERT(t, binary.size() == 15);
    return kTR_Pass;
}

DLL_EXPORT int test_stmtemitter_structref(ITesting *t) {
    const char srcCode[]= {
            "struct table {\n"\
        "   some_byte dc.b 1\n"\
        "   some_word dc.w 1\n"\
        "   some_dword dc.d 1\n"\
        "   some_long dc.l 1\n"\
        "}\n"\
        "  move.l (a0+table.some_byte),d0\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);
    auto binary = compiler.Data();
    // TR_ASSERT(t, binary.size() == 123);
    return kTR_Pass;

}


DLL_EXPORT int test_stmtemitter_struct_in_struct(ITesting *t) {
    // Should generate 16 bytes
    // 1+2+4+8 = 15

    const char srcCode[]= {
            "struct table {\n"\
        "   some_byte dc.b 1\n"\
        "   some_word dc.w 1\n"\
        "   some_dword dc.d 1\n"\
        "   some_long dc.l 1\n"\
        "}\n"\
        ""
            "struct table_2 {\n"\
        "   tableref dc.struct table,1\n"\
        "}\n"\
        "data: dc.struct table {}"
            ""
    };

    Parser parser;
    Compiler compiler;

    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
//    TR_ASSERT(t, binary == expectedBinary);

    TR_ASSERT(t, binary.size() == 15);
    return kTR_Pass;
}
