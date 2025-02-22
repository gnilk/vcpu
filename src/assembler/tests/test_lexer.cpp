//
// Created by gnilk on 14.12.23.
//

#include <testinterface.h>
#include "Lexer/TextLexer.h"

using namespace gnilk;

extern "C" {
    DLL_EXPORT int test_lexer(ITesting *t);
    DLL_EXPORT int test_lexer_keywords(ITesting *t);
    DLL_EXPORT int test_lexer_number(ITesting *t);
    DLL_EXPORT int test_lexer_linecomments(ITesting *t);
}

DLL_EXPORT int test_lexer(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_lexer_number(ITesting *t) {
    gnilk::Lexer lexer;

    auto tokens = lexer.Tokenize("0");
    TR_ASSERT(t, tokens.size() > 0);
    TR_ASSERT(t, tokens[0].type == gnilk::TokenType::Number);
    TR_ASSERT(t, tokens[0].value == "0");


    tokens = lexer.Tokenize("10");
    TR_ASSERT(t, tokens.size() > 0);
    TR_ASSERT(t, tokens[0].type == gnilk::TokenType::Number);
    TR_ASSERT(t, tokens[0].value == "10");

    tokens = lexer.Tokenize("0xdeadbeef");
    TR_ASSERT(t, tokens.size() > 0);
    TR_ASSERT(t, tokens[0].type == gnilk::TokenType::NumberHex);
    TR_ASSERT(t, tokens[0].value == "0xdeadbeef");

    tokens = lexer.Tokenize("0o1234");
    TR_ASSERT(t, tokens.size() > 0);
    TR_ASSERT(t, tokens[0].type == gnilk::TokenType::NumberOctal);
    TR_ASSERT(t, tokens[0].value == "0o1234");

    tokens = lexer.Tokenize("0b10110011");
    TR_ASSERT(t, tokens.size() > 0);
    TR_ASSERT(t, tokens[0].type == gnilk::TokenType::NumberBinary);
    TR_ASSERT(t, tokens[0].value == "0b10110011");



    return kTR_Pass;

}

DLL_EXPORT int test_lexer_keywords(ITesting *t) {
    Lexer lexer;
    auto tokens = lexer.Tokenize("move.l d0,1");
    TR_ASSERT(t, tokens[0].type == TokenType::Identifier);
    TR_ASSERT(t, tokens[1].type == TokenType::Dot);
    TR_ASSERT(t, tokens[2].type == TokenType::OpSize);
    TR_ASSERT(t, tokens[3].type == TokenType::DataReg);
    TR_ASSERT(t, tokens[4].type == TokenType::Comma);
    TR_ASSERT(t, tokens[5].type == TokenType::Number);
    return kTR_Pass;
}

DLL_EXPORT int test_lexer_linecomments(ITesting *t) {
    const char srcCode[]= {
        "; -- this is a comment\n"\
        "; one more\n"\
        "\tmove d0,d1   ; number two\n"\
        "\tlabel:\n"\
        ""
    };

    std::string_view strSource(srcCode, sizeof(srcCode));
    Lexer lexer;
    auto tokens = lexer.Tokenize(strSource);
    TR_ASSERT(t, tokens[0].type == TokenType::LineComment);
    TR_ASSERT(t, tokens[1].type == TokenType::CommentedText);
    //TR_ASSERT(t, tokens[2].type == TokenType::EoL);

    return kTR_Pass;
}


