//
// Created by gnilk on 20.01.25.
//
#include <testinterface.h>
#include "Lexer/TextLexer.h"
#include "Parser/Parser.h"
#include "Preprocessor/PreProcessor.h"

using namespace gnilk;
using namespace gnilk::assembler;

extern "C" {
DLL_EXPORT int test_preproc(ITesting *t);
DLL_EXPORT int test_preproc_process(ITesting *t);
}

DLL_EXPORT int test_preproc(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_preproc_process(ITesting *t) {

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

    Lexer lexer;
    auto tokens = lexer.Tokenize(src1);

    bool bOk = preProcessor.Process(tokens, assetLoader);
    TR_ASSERT(t, bOk);

    return kTR_Pass;
}
