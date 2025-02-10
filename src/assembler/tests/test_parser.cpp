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
    DLL_EXPORT int test_parser_linecomments(ITesting *t);
    DLL_EXPORT int test_parser_meta(ITesting *t);
    DLL_EXPORT int test_parser_struct(ITesting *t);
    DLL_EXPORT int test_parser_struct_in_struct(ITesting *t);
    DLL_EXPORT int test_parser_structref(ITesting *t);
    DLL_EXPORT int test_parser_const(ITesting *t);
    DLL_EXPORT int test_parser_expressions(ITesting *t);
    DLL_EXPORT int test_parser_public(ITesting *t);
    DLL_EXPORT int test_parser_preproc(ITesting *t);
}
DLL_EXPORT int test_parser(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_parser_instruction(ITesting *t) {
    Parser parser;

    auto ast = parser.ProduceAST("move.l cr0, d0");
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();

    ast = parser.ProduceAST("move.l d0,0x45");
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
DLL_EXPORT int test_parser_linecomments(ITesting *t) {
    const char srcCode[]= {
        "; -- this is a comment\n"\
        "; one more\n"\
        "\tmove d0,d1   ; number two\n"\
        "\tlabel:\n"\
        ""
    };

    std::string_view strSource(srcCode, sizeof(srcCode));

    Parser parser;
    auto ast = parser.ProduceAST(strSource);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}
DLL_EXPORT int test_parser_meta(ITesting *t) {
    const char srcCode[]= {
        ".data\n"\
        ".org 0x100 \n"\
        ".code\n"\
        "\tmove d0,d1\n"\
        ".data\n"\
        "\tlabel: dc.b 0x101\n"\
        ""
    };
    std::string_view strSource(srcCode, sizeof(srcCode));

    Parser parser;
    auto ast = parser.ProduceAST(strSource);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;

}

DLL_EXPORT int test_parser_struct(ITesting *t) {
    const char srcCode[]= {
        "struct table {\n"\
        "   some_byte dc.b 1\n"\
        "   some_word dc.w 1\n"\
        "   some_dword dc.d 1\n"\
        "   some_long dc.l 1\n"\
        "}\n"\
        ""
    };

    Parser parser;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}

DLL_EXPORT int test_parser_struct_in_struct(ITesting *t) {
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
        ""
    };

    Parser parser;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}


DLL_EXPORT int test_parser_structref(ITesting *t) {
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
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;

}

DLL_EXPORT int test_parser_const(ITesting *t) {
    const char srcCode[]= {
        "const CONSTANT 0x24\n"\
        "const NEXT  CONSTANT\n"\
        ""
    };
    Parser parser;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}

DLL_EXPORT int test_parser_expressions(ITesting *t) {
    const char srcCode[]= {
        "move.l d0, (a0+4)\n"\
        "move.l d0, 4+5\n"\
        "\n"\
        ""
    };

    Parser parser;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;

}

DLL_EXPORT int test_parser_public(ITesting *t) {
    const char srcCode[]= {
            ".code\n"\
            "public myfunc\n"\
            "myfunc:\n"\
            "  move.l d0,0x42\n"
    };

    Parser parser;
    auto ast = parser.ProduceAST(srcCode);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}


DLL_EXPORT int test_parser_preproc(ITesting *t) {

    std::string src3 = "// this is another src \n"\
                        "const SRC_3_CONST 4712 \n"\
                        "";
    std::string src2 = "// this is another src \n"\
                       "// with another include \n"\
                       "   .include \"src3.asm\" \n"\
                       "const SRC_2_CONST 4711 \n"\
                       "";
    std::string src1 = ".include \"src2.asm\" \n"\
                       ".code \n"\
                       "move.l d0, SRC_3_CONST \n"\
                       "move.l d1, SRC_2_CONST \n"\
                       "nop\n"\
                       "brk\n";

    PreProcessor preProcessor;

    std::string processedString;
    auto assetLoader = [src2,src3](std::string &out, const std::string &assetName, int flags) {
        if (assetName == "src2.asm") {
            for (auto &s: src2) {
                out += s;
            }
        } else if (assetName == "src3.asm") {
            for (auto &s: src3) {
                out += s;
            }
        } else {
            return false;
        }
        return true;
    };

    Parser parser;
    parser.SetAssetLoader(assetLoader);
    auto ast = parser.ProduceAST(src1);
    TR_ASSERT(t, ast != nullptr);
    ast->Dump();
    return kTR_Pass;
}