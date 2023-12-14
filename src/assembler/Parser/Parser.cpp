//
// Created by gnilk on 14.12.23.
//

#include "Parser.h"

using namespace gnilk;
using namespace gnilk::assembler;

ast::Program::Ref Parser::ProduceAST(const std::string&srcCode) {
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
        case TokenType::Instruction :
            return ParseInstruction();
    }
    return nullptr;
}


ast::Statement::Ref Parser::ParseInstruction() {
    auto operand = At().value;
    if (operand != "move") {
        fmt::println(stderr, "unsupported instructions {}", At().value);
        return nullptr;
    }

    auto instrStatment = std::make_shared<ast::MoveInstrStatment>(operand);

    Eat();
    if (At().type == TokenType::OpSize) {
        auto opSize = Eat();
        static std::unordered_map<std::string, OperandSize> strToOpSize = {
            {".b", OperandSize::Byte},
            {".w", OperandSize::Word},
            {".d", OperandSize::DWord},
            {".l", OperandSize::Long},
        };
        if (!strToOpSize.contains(opSize.value)) {
            fmt::println(stderr, "Unsupported operand size {}", opSize.value);
            return nullptr;
        }
        instrStatment->SetOpSize(strToOpSize[opSize.value]);
    }
    auto dst = Eat();
    if ((dst.type != TokenType::DataReg) && (dst.type != TokenType::AddressReg)) {
        fmt::println(stderr, "Destination must be DataRegister or AddressRegister, {} is neither", dst.value);
        return nullptr;
    }
    instrStatment->SetDst(dst.value);


    Expect(TokenType::Comma, "Operand requires two arguments!");
    // Fixme: Allow 'expression' here...
    auto src = Eat();
    instrStatment->SetSrc(src.value);

    return instrStatment;
}


///////////
const Token &Parser::Expect(TokenType expectedType, const std::string &errmsg) {
    auto &prev = Eat();
    if (prev.type != expectedType) {
        fmt::println(stderr, "Parser Error: {}, got token: {} (type: {}) - expecting type: {}", errmsg, prev.value, (int)prev.type, (int)expectedType);
        // FIXME: Don't exit here!
        exit(1);
    }
    return prev;
}



