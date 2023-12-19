//
// Created by gnilk on 15.12.23.
//

#include <stdio.h>
#include <filesystem>

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Compiler/Compiler.h"

using namespace gnilk::assembler;

extern "C" {
    DLL_EXPORT int test_compiler(ITesting *t);
    DLL_EXPORT int test_compiler_file(ITesting *t);
    DLL_EXPORT int test_compiler_move_immediate(ITesting *t);
    DLL_EXPORT int test_compiler_move_reg2reg(ITesting *t);
    DLL_EXPORT int test_compiler_add_immediate(ITesting *t);
    DLL_EXPORT int test_compiler_add_reg2reg(ITesting *t);
    DLL_EXPORT int test_compiler_nop(ITesting *t);
    DLL_EXPORT int test_compiler_push(ITesting *t);
    DLL_EXPORT int test_compiler_pop(ITesting *t);
    DLL_EXPORT int test_compiler_callrelative(ITesting *t);
    DLL_EXPORT int test_compiler_calllabel(ITesting *t);
    DLL_EXPORT int test_compiler_lea_label(ITesting *t);
    DLL_EXPORT int test_compiler_move_indirect(ITesting *t);
    DLL_EXPORT int test_compiler_array_bytedecl(ITesting *t);
    DLL_EXPORT int test_compiler_array_worddecl(ITesting *t);
    DLL_EXPORT int test_compiler_var_bytedecl(ITesting *t);
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

    TR_ASSERT(t, compiler.GenerateCode(ast));

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
        TR_ASSERT(t, compiler.GenerateCode(ast));
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
    TR_ASSERT(t, compiler.GenerateCode(ast));
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
        TR_ASSERT(t, compiler.GenerateCode(ast));
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
        TR_ASSERT(t, compiler.GenerateCode(ast));
        auto binary = compiler.Data();
        if (binary != expectedBinaries[i]) {
            printf("Failed %zu (%s)\n", i, codes[i].c_str());
        }
        TR_ASSERT(t, binary == expectedBinaries[i]);
    }
    return kTR_Pass;
}

DLL_EXPORT int test_compiler_callrelative(ITesting *t) {
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
    auto res = compiler.GenerateCode(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_calllabel(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
        0xc0,0x03,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0d,        // 0, Call label, opSize = lword, [reg|mode] = 0|abs, <address of label> = 0x0d
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
    auto res = compiler.GenerateCode(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;
}

DLL_EXPORT int test_compiler_lea_label(ITesting *t) {
    std::vector<uint8_t> expectedBinary= {
        0x28,0x03,0x83,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,        // 0, Call label, opSize = lword, [reg|mode] = 0|abs, <address of label> = 0x0d
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
    auto res = compiler.GenerateCode(ast);
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
    auto res = compiler.GenerateCode(ast);
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
    auto res = compiler.GenerateCode(ast);
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
    auto res = compiler.GenerateCode(ast);
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
    auto res = compiler.GenerateCode(ast);
    auto binary = compiler.Data();
    TR_ASSERT(t, binary == expectedBinary);

    return kTR_Pass;

}

