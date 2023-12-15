//
// Created by gnilk on 14.12.23.
//

#include "Parser.h"
/*
 * TO-OD:
 * - Move over Expression and Literals from 'astparser' project
 * - Add 'register-literal' to Literals
 * - Add more or less all AST handling from Expression and onwards; this enables: "move d0, <expression>"
 */


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
    // Swap out below for:
    // dst = ParseExpression();

    auto dst = ParseExpression();
    if (dst->Kind() != ast::NodeType::kRegisterLiteral) {
        fmt::println(stderr, "Destination must be DataRegister or AddressRegister");
        return nullptr;
    }
    instrStatment->SetDst(dst);

    // literal must be separated by ','
    Expect(TokenType::Comma, "Operand requires two arguments!");


    // Fixme: replace with: src = ParseExpression();
    auto src = ParseExpression();
    instrStatment->SetSrc(src);

    // Expressions are a little different, like:
    //   move d0, a0             <- move reg to reg
    //   move d0, (a0)           <- move dereferenced a0 to d0
    //   move d0, (a0 + d1)      <- move relative to register, need to check op codes for this
    //   move d0, (a0 + d1<<1)   <- move relative, but shift first
    //   move d0, (a0 + offset)  <- move relative to abs. value
    //
    // also reversed/combined is possible:
    //   move (a0+d1), (a1+d2)
    //
    // for add (?):
    //    add.w d0, d1, d2        ; d0 = d1 + d2
    //


    return instrStatment;
}

ast::Expression::Ref Parser::ParseExpression() {
    return ParsePrimaryExpression();
}

ast::Expression::Ref Parser::ParsePrimaryExpression() {
    auto tokenType = At().type;
    switch(tokenType) {
        case TokenType::DataReg :
            return ast::RegisterLiteral::Create(Eat().value);
        case TokenType::AddressReg :
            return ast::RegisterLiteral::Create(Eat().value);
        case TokenType::Number :
            return ast::NumericLiteral::Create(Eat().value);
        case TokenType::NumberHex :
            return ast::NumericLiteral::CreateFromHex(Eat().value);
    }
    fmt::println(stderr, "Unexpected token found: {}",At().value.c_str());
    return nullptr;
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



