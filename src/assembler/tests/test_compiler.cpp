//
// Created by gnilk on 15.12.23.
//

#include <stdio.h>
#include <filesystem>

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Compiler/Compiler.h"
#include "Linker/CompiledUnit.h"
#include "HexDump.h"

using namespace gnilk::assembler;

extern "C" {
    DLL_EXPORT int test_compiler(ITesting *t);
    DLL_EXPORT int test_compiler_linksimple(ITesting *t);
    DLL_EXPORT int test_compiler_file(ITesting *t);
    DLL_EXPORT int test_compiler_move_immediate(ITesting *t);
    DLL_EXPORT int test_compiler_move_reg2reg(ITesting *t);
    DLL_EXPORT int test_compiler_move_relative(ITesting *t);
    DLL_EXPORT int test_compiler_move_control(ITesting *t);
    DLL_EXPORT int test_compiler_add_immediate(ITesting *t);
    DLL_EXPORT int test_compiler_add_reg2reg(ITesting *t);
    DLL_EXPORT int test_compiler_nop(ITesting *t);
    DLL_EXPORT int test_compiler_push(ITesting *t);
    DLL_EXPORT int test_compiler_pop(ITesting *t);
    DLL_EXPORT int test_compiler_call_relative(ITesting *t);
    DLL_EXPORT int test_compiler_call_label(ITesting *t);
    DLL_EXPORT int test_compiler_call_relative_label(ITesting *t);
    DLL_EXPORT int test_compiler_call_backrelative_label(ITesting *t);
    DLL_EXPORT int test_compiler_lea_label(ITesting *t);
    DLL_EXPORT int test_compiler_move_indirect(ITesting *t);
    DLL_EXPORT int test_compiler_array_bytedecl(ITesting *t);
    DLL_EXPORT int test_compiler_array_worddecl(ITesting *t);
    DLL_EXPORT int test_compiler_var_bytedecl(ITesting *t);
    DLL_EXPORT int test_compiler_const_literal(ITesting *t);
    DLL_EXPORT int test_compiler_const_string(ITesting *t);
    DLL_EXPORT int test_compiler_structref(ITesting *t);
    DLL_EXPORT int test_compiler_orgdecl(ITesting *t);
    DLL_EXPORT int test_compiler_addtovar(ITesting *t);
    DLL_EXPORT int test_compiler_cmpbne(ITesting *t);
}

DLL_EXPORT int test_compiler(ITesting *t) {
    t->CaseDepends("add_immediate", "move_immediate");
    t->CaseDepends("add_reg2reg", "move_reg2reg");
    return kTR_Pass;
}
DLL_EXPORT int test_compiler_file(ITesting *t) {

    std::filesystem::path pathToSrcFile("Assets/test.asm");
    size_t szFile = file_size(pathToSrcFile);
    char *data = (char *)malloc(szFile + 10);
    if (data == nullptr) {
        return false;
    }
    memset(data, 0, szFile + 10);
    std::string_view strData(data, szFile);


    FILE *f = fopen("Assets/test.asm", "r+");
    TR_ASSERT(t, f != nullptr);
    auto nRead = fread(data, 1, szFile, f);
    fclose(f);

    gnilk::assembler::Parser parser;
    gnilk::assembler::Compiler compiler;

    auto ast = parser.ProduceAST(strData);
    TR_ASSERT(t, ast != nullptr);

    TR_ASSERT(t, compiler.CompileAndLink(ast));

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_linksimple(ITesting* t) {
    Parser parser;
    Compiler compiler;

    // move.b d0, 0x45
    static std::vector<uint8_t> expectedCase1 = {0x20,0x00,0x03,0x01, 0x45};
    auto prg = parser.ProduceAST("move.b d0,0x45");
    auto res = compiler.CompileAndLink(prg);
    TR_ASSERT(t, res);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase1);

    return kTR_Pass;
}


DLL_EXPORT int test_compiler_move_immediate(ITesting *t) {
    Parser parser;
    Compiler compiler;

    // move.b d0, 0x45
    static std::vector<uint8_t> expectedCase1 = {0x20,0x00,0x03,0x01, 0x45};
    auto prg = parser.ProduceAST("move.b d0,0x45");
    auto res = compiler.CompileAndLink(prg);
    TR_ASSERT(t, res);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase1);
    binary.clear();

    // move.w d0, 0x4433
    static std::vector<uint8_t> expectedCase2 = {0x20,0x01,0x03,0x01, 0x44,0x33};
    prg = parser.ProduceAST("move.w d0, 0x4433");
    TR_ASSERT(t, compiler.CompileAndLink(prg));
    binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase2);

    // move.d d0, 0x44332211
    static std::vector<uint8_t> expectedCase3 = {0x20,0x02,0x03,0x01, 0x44,0x33,0x22,0x11};
    prg = parser.ProduceAST("move.d d0, 0x44332211");
    TR_ASSERT(t, compiler.CompileAndLink(prg));
    binary = compiler.Data();
    TR_ASSERT(t, binary == expectedCase3);

    // LWORD currently not supported by AST tree...

    // Different register

    // move.b d2, 0x44
    static std::vector<uint8_t> expectedCase4 = {0x20,0x00,0x23,0x01, 0x44};
    prg = parser.ProduceAST("move.b d2, 0x44");
    TR_ASSERT(t, compiler.CompileAndLink(prg));
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
        TR_ASSERT(t, compiler.CompileAndLink(ast));
        auto binary = compiler.Data();
        TR_ASSERT(t, binary == binaries[i]);
    }

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_move_relative(ITesting *t) {
    // The following test-cases are taken from the test_vcpu.cpp
    std::vector<uint8_t> binaries[]= {
        {0x20,0x03,0x98,0x00,0x03},        // move.l (a1+0),d0
        {0x20,0x01,0x94,0x43, 0x84,0x31},  // move.w (a1+d4<<3), (a0+d3<<1)
        {0x20,0x01,0x03,0x84,0x32},        // move.w d0, (a0+d3<<2)
        {0x20,0x01,0x03,0x88,0x47},        // move.w d0, (a0+0x47)
        {0x20,0x01,0x03,0x80},             // move.w d0, (a0)
        {0x20,0x01,0x03,0x84,0x30},        // move.w d0, (a0+d3)

    };

    std::vector<std::string> code={
        {"move.l (a1+0),d0"},
        {"move.w (a1+d4<<3), (a0+d3<<1)"},
        {"move.w d0, (a0 + d3<<2)"},
        {"move.w d0, (a0 + 0x47)"},
        {"move.w d0, (a0)"},
        {"move.w d0, (a0 + d3)"},
    };

    Parser parser;
    Compiler compiler;

    for(int i=0;i<code.size();i++) {
        auto ast = parser.ProduceAST(code[i]);
        TR_ASSERT(t, ast != nullptr);
        TR_ASSERT(t, compiler.CompileAndLink(ast));
        auto binary = compiler.Data();
        for(auto &v : binary) {
            printf("0x%.2x ", v);
        }
        printf("\n");
        TR_ASSERT(t, binary == binaries[i]);
    }

    return kTR_Pass;



    return kTR_Pass;
}
DLL_EXPORT int test_compiler_move_control(ITesting *t) {
    // The following test-cases are taken from the test_vcpu.cpp
    std::vector<uint8_t> binaries[]= {
        {0x20,0x13,0x83,0x03},    // move.l cr0, d0
        {0x20,0x13,0x03,0x83},    // move.l d0, cr0
    };

    std::vector<std::string> code={
        {"move.l cr0, d0"},
        {"move.l d0, cr0"},
    };

    Parser parser;
    Compiler compiler;

    for(int i=0;i<code.size();i++) {
        auto ast = parser.ProduceAST(code[i]);
        TR_ASSERT(t, ast != nullptr);
        TR_ASSERT(t, compiler.CompileAndLink(ast));
        auto binary = compiler.Data();
        TR_ASSERT(t, binary == binaries[i]);
    }

    return kTR_Pass;
}



DLL_EXPORT int test_compiler_add_immediate(ITesting *t) {
    std::vector<uint8_t> binaries[]= {
        {0x30,0x00,0x03,0x01, 0x44},        // add.b d0, 0x44
        {0x30,0x01,0x03,0x01, 0x44,0x33},   // add.w d0, 0x4433
    };

    std::vector<std::string> code={
        {"add.b d0, 0x44"},
        {"add.w d0, 0x4433"},
    };

    Parser parser;
    Compiler compiler;

    for(int i=0;i<code.size();i++) {
        auto ast = parser.ProduceAST(code[i]);
        TR_ASSERT(t, ast != nullptr);
        TR_ASSERT(t, compiler.CompileAndLink(ast));
        auto binary = compiler.Data();
        TR_ASSERT(t, binary == binaries[i]);
    }

    return kTR_Pass;
}
DLL_EXPORT int test_compiler_add_reg2reg(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_compiler_nop(ITesting *t) {
    std::vector<uint8_t> expectedBinary = {
        0xf1
    };
    std::vector<std::string> code={
        {"nop"},
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast != nullptr);
    TR_ASSERT(t, compiler.CompileAndLink(ast));
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_push(ITesting *t) {
    std::vector<uint8_t> expectedBinaries[]= {
        {0x70,0x00,0x01,0x80},                       // push.b 0x43
        {0x70,0x01,0x01,0x80,0x70},                  // push.w 0x4200
        {0x70,0x02,0x01,0x80,0x70,0x60,0x50},        // push.d 0x41000000
        {0x70,0x00,0x03},                            // push.b d0
        {0x70,0x01,0x13},                            // push.w d1
        {0x70,0x02,0x23},                            // push.d d2

    };

    std::vector<std::string> codes={
        {"push.b  0x80"},
        {"push.w  0x8070"},
        {"push.d  0x80706050"},
        {"push.b    d0"},
        {"push.w    d1"},
        {"push.d    d2"},
    };
    Parser parser;
    Compiler compiler;
    for(size_t i=0;i<codes.size();i++) {
        auto ast = parser.ProduceAST(codes[i]);
        TR_ASSERT(t, ast != nullptr);
        TR_ASSERT(t, compiler.CompileAndLink(ast));
        auto binary = compiler.Data();
        if (binary != expectedBinaries[i]) {
            printf("Failed %zu (%s)\n", i, codes[i].c_str());
        }
        TR_ASSERT(t, binary == expectedBinaries[i]);
    }
    return kTR_Pass;
}

DLL_EXPORT int test_compiler_pop(ITesting *t) {
    std::vector<uint8_t> expectedBinaries[]= {
        {0x80,0x02,0x03},                            // pop.d d0
        {0x80,0x01,0x03},                            // pop.w d0
        {0x80,0x00,0x03},                            // pop.b d0
        {0x80,0x02,0x03},                            // pop.d d0
        {0x80,0x01,0x13},                            // pop.w d1
        {0x80,0x00,0x23},                            // pop.b d2
    };

    std::vector<std::string> codes={
        {"pop.d     d0"},
        {"pop.w     d0"},
        {"pop.b     d0"},
        {"pop.d     d0"},
        {"pop.w     d1"},
        {"pop.b     d2"},
  };
    Parser parser;
    Compiler compiler;
    for(size_t i=0;i<codes.size();i++) {
        auto ast = parser.ProduceAST(codes[i]);
        TR_ASSERT(t, ast != nullptr);
        TR_ASSERT(t, compiler.CompileAndLink(ast));
        auto binary = compiler.Data();
        if (binary != expectedBinaries[i]) {
            printf("Failed %zu (%s)\n", i, codes[i].c_str());
        }
        TR_ASSERT(t, binary == expectedBinaries[i]);
    }
    return kTR_Pass;
}

DLL_EXPORT int test_compiler_call_relative(ITesting *t) {
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

// Test of forward relative
DLL_EXPORT int test_compiler_call_relative_label(ITesting *t) {
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
            "call.b    label\n "\
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

// Test of forward relative
DLL_EXPORT int test_compiler_call_backrelative_label(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
        0x90,0x00,0x03,0x01,0x33,       // cmp.b d0, 0x33
        0xd0,0x00,0x01,0xf7,            // beq.b -9
        0x00,
    };
    std::vector<std::string> codes={
        {
            "label:\n"\
            "cmp.b d0, 0x33\n"\
            "beq.b label\n"\
            "brk"
        }
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(codes[0]);
    TR_ASSERT(t, ast != nullptr);
    auto compileOk = compiler.CompileAndLink(ast);
    TR_ASSERT(t, compileOk == true);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}


DLL_EXPORT int test_compiler_call_label(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
        0xc0,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0d,        // 0, Call label, opSize = lword, [reg|mode] = 0|immediate, <address of label> = 0x0d
        0xf1,                       // 4
        0x00,                       // 5 WHALT!
        0xf1,                       // 6 <- call should go here (offset of label)
        0xf1,                       // 7
        0xf0,                       // 8 <- return, should be ip+1 => 5
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

DLL_EXPORT int test_compiler_lea_label(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
        0x28,0x03,0x83,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,        // 0, Call label, opSize = lword, [reg|mode] = 0|abs, <address of label> = 0x0d
        0xf1,                       // 4
        0x00,                       // 5 WHALT!
        0xf1,                       // 6 <- call should go here (offset of label)
        0xf1,                       // 7
        0xf0,                       // 8 <- return, should be ip+1 => 5
    };

    std::vector<std::string> codes={
        {
            "lea    a0,label\n "\
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
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;

}

DLL_EXPORT int test_compiler_move_indirect(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
        0x28,0x03,0x83,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,        // 0, Call label, opSize = lword, [reg|mode] = 0|abs, <address of label> = 0x0d
        0x20,0x00,0x03,0x80,                                                // this is wrong!
        0x00,                       // 5 WHALT!
        0xf1,                       // 6 <- call should go here (offset of label)
        0xf1,                       // 7
        0xf0,                       // 8 <- return, should be ip+1 => 5
    };

    std::vector<std::string> codes={
        {
            "lea    a0,label \n "\
            "move.b    d0,(a0)     \n"\
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
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;

}

DLL_EXPORT int test_compiler_array_bytedecl(ITesting *t) {
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
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_array_worddecl(ITesting *t) {
    std::vector<uint8_t> expectedBinary = {
        0,0, 0,1, 0,2, 0,3,
    };
    std::vector<std::string> code={
        {"data: dc.w 0,1,2,3"}
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_var_bytedecl(ITesting *t) {
    std::vector<uint8_t> expectedBinary = {
        0xff
    };
    std::vector<std::string> code={
        {"data: dc.b 0xff"}
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(code[0]);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;

}

DLL_EXPORT int test_compiler_const_literal(ITesting *t) {
    std::vector<uint8_t> expectedBinary = {
        0x20,0x00,0x03,0x01, 0x44,
        0x20,0x00,0x13,0x01, 0x44,
    };

    const char srcCode[]= {
        "const CONSTANT (0x24+32)\n"\
        "const NEXT  CONSTANT\n"\
        "move.b d0, CONSTANT\n"\
        "move.b d1, NEXT\n"\
        ""
    };
    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;

}
DLL_EXPORT int test_compiler_const_string(ITesting *t) {
    return kTR_Pass;

    // This is not yet supported!
    // std::vector<uint8_t> expectedBinary = {
    // };
    //
    // const char srcCode[]= {
    //     "const CONSTANT \"this is a string\"\n"\
    //     "lea a0, CONSTANT\n"\
    //     ""
    // };
    // Parser parser;
    // Compiler compiler;
    // auto ast = parser.ProduceAST(srcCode);
    // TR_ASSERT(t, ast != nullptr);
    // auto res = compiler.CompileAndLink(ast);
    // TR_ASSERT(t, res);
    // auto bin = compiler.Data();
    //
    // return kTR_Pass;

}
DLL_EXPORT int test_compiler_structref(ITesting *t) {
    const char srcCode[]= {
        "struct table {\n"\
        "   some_byte rs.b 1\n"\
        "   some_word rs.w 1\n"\
        "   some_dword rs.d 1\n"\
        "   some_long rs.l 1\n"\
        "}\n"\
        "  move.l (a0+table.some_byte),d0\n"\
        "  move.l (a0+table.some_word),d0\n"\
        "  move.l (a0+table.some_dword),d0\n"\
        "  move.l (a0+table.some_long),d0\n"\
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

    std::vector<uint8_t> binaries[]= {
        {
            0x20,0x03,0x88,0x00,0x03,
            0x20,0x03,0x88,0x01,0x03,
            0x20,0x03,0x88,0x03,0x03,
            0x20,0x03,0x88,0x07,0x03,   // <- very unaligned
        },
    };

    TR_ASSERT(t, binary == binaries[0]);

    return kTR_Pass;

}

DLL_EXPORT int test_compiler_orgdecl(ITesting *t) {
    const char srcCode[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "   dc.b 0xaa,0xbb,0xcc,0xdd\n"\
        "   .org 100\n"\
        "   move.b d0, 1\n"\
        "   .org 200\n"\
        "   move.b d0, 1\n"\
        "   ret\n"\
        ""
    };
    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto &unit = compiler.GetCompiledUnit();
    std::vector<Segment::Ref> segments;
    unit.GetSegments(segments);

    // NOTE: Don't assert on number of segments - there are 'hidden' segments created for the benefit of the linker...

    static size_t expectedLoadAddr[]={0,100,200};

    printf("Segments, num=%d\n", (int)segments.size());
    for(int idxSeg=0;idxSeg<segments.size();idxSeg++) {
        auto &s = segments[idxSeg];
        printf("  Name=%s\n", s->Name().c_str());
        auto &chunks = s->DataChunks();
        TR_ASSERT(t, chunks.size() == 3);

        printf("  Chunks, num=%d\n", (int)chunks.size());
        for (int idxChunk=0;idxChunk<chunks.size();idxChunk++) {
            auto &c = chunks[idxChunk];
            if (s->Name() == ".text") {
                TR_ASSERT(t, c->LoadAddress() == expectedLoadAddr[idxChunk]);
            }
            printf("    %d\n", idxChunk);
            printf("    LoadAddress=0x%x (%d)\n",(int)c->LoadAddress(), (int)c->LoadAddress());
            printf("    EndAddress=0x%x (%d)\n",(int)c->EndAddress(), (int)c->EndAddress());
            printf("    Bytes=0x%x (%d)\n",(int)c->Size(), (int)c->Size());
        }
    }

    auto data = compiler.Data();
    printf("Binary Size: %d\n", (int)data.size());
    HexDump::ToConsole(data.data()+0, 16);
    HexDump::ToConsole(data.data()+100, 16);
    HexDump::ToConsole(data.data()+200, 16);
    return kTR_Pass;
}
DLL_EXPORT int test_compiler_addtovar(ITesting *t) {
    const char srcCode[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "   add.l counter,1\n"\
        "   ret\n"\
        "   .data\n"\
        "   counter: dc.l 0\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto data = compiler.Data();
    printf("Binary:\n");
    HexDump::ToConsole(data.data(),data.size());


    return kTR_Pass;
}

DLL_EXPORT int test_compiler_cmpbne(ITesting *t) {
    const char srcCode[]= {
        "  .code \n"\
        "   .org 0x0000 \n"\
        "lp: \n"\
        "   cmp.l counter,1\n"\
        "   bne lp\n"\
        "   ret\n"\
        "   .data\n"\
        "   counter: dc.l 0\n"\
        ""
    };

    Parser parser;
    Compiler compiler;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    auto res = compiler.CompileAndLink(ast);
    TR_ASSERT(t, res);

    auto data = compiler.Data();
    printf("Binary:\n");
    HexDump::ToConsole(data.data(),data.size());

    return kTR_Pass;
}

