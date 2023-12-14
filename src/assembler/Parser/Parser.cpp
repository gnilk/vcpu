//
// Created by gnilk on 14.12.23.
//

#include "Parser.h"

using namespace gnilk;
using namespace gnilk::assembler;

ast::Program::Ref Parser::ProduceCode(const std::string&srcCode) {
    auto program = Begin(srcCode);
    while(!Done()) {
        auto stmt = ParseStatement();
        // we could expect ';' here?
        if (stmt == nullptr) {
            return nullptr;
        }
        program->Append(stmt);
    }
    return program;
}

ast::Program::Ref Parser::Begin(const std::string &srcCode) {
    tokens = lexer.Tokenize(srcCode);
    it = tokens.begin();
    return std::make_shared<ast::Program>();
}

ast::Statement::Ref Parser::ParseStatement() {
    switch(At().type) {

    }

}





