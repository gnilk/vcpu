//
// Created by gnilk on 14.12.23.
//

#include <map>
#include <unordered_set>
#include "Parser.h"
#include "InstructionSet.h"
/*
 * TO-OD:
 * - Move over Expression and Literals from 'astparser' project
 * - Add 'register-literal' to Literals
 * - Add more or less all AST handling from Expression and onwards; this enables: "move d0, <expression>"
 */


using namespace gnilk;
using namespace gnilk::assembler;

ast::Program::Ref Parser::ProduceAST(const std::string_view &srcCode) {
    auto program = Begin(srcCode);
    while(!Done()) {
        // EOL statements - we just continue
        if (At().type == TokenType::EoL) {
            Eat();
            continue;
        }

        auto stmt = ParseStatement();
        // we could expect ';' here?
        if (stmt == nullptr) {
            return nullptr;
        }
        program->Append(stmt);
    }
    return program;
}

ast::Program::Ref Parser::Begin(const std::string_view &srcCode) {
    tokens = lexer.Tokenize(srcCode);
    it = tokens.begin();
    return std::make_shared<ast::Program>();
}

ast::Statement::Ref Parser::ParseStatement() {
    switch(At().type) {
        case TokenType::Dot :
            return ParseMetaStatement();
        case TokenType::Identifier :
            return ParseIdentifierOrInstr();
        case TokenType::Declaration :
            return ParseDeclaration();
        case TokenType::LineComment :
            return ParseLineComment();
    }
    return nullptr;
}

ast::Statement::Ref Parser::ParseMetaStatement() {
    Eat();
    if (At().type != TokenType::Identifier) {
        fmt::println(stderr,"Identifier expected in meta statement; .<meta>, got: {}", At().value);
        return nullptr;
    }
    auto &symbol = At().value;

    // Really validate this already here - or push this to compiler stage?
    static std::unordered_set<std::string> validMetaSymbols = {
        "org",
        "code",
        "data",
        "text",
    };

    if (!validMetaSymbols.contains(symbol)) {
        fmt::println(stderr,"Invalid/Unsupported meta directive: {}", symbol);
        return nullptr;
    }


    auto meta = std::make_shared<ast::MetaStatement>(symbol);
    Eat();

    if (symbol == "org") {
        auto optExp = ParseExpression();
        if (optExp == nullptr) {
            fmt::println(stderr, "expected expression after '.org', like: .org 0x100");
            return nullptr;
        }
        meta->SetOptional(optExp);
    }

    return meta;
}

ast::Statement::Ref Parser::ParseLineComment() {
    Eat();
    auto text = Expect(TokenType::CommentedText, "Line comment should be followed by commented text");
    Expect(TokenType::EoL, "Should be end of line");
    return std::make_shared<ast::LineComment>(text.value);
}

std::pair<bool, vcpu::OperandSize> Parser::ParseOpSize() {
    if (At().type != TokenType::Dot) {
        return {false,{}};
    }
    Eat();

    auto opSize = Eat();
    static std::unordered_map<std::string, gnilk::vcpu::OperandSize> strToOpSize = {
        {"b", gnilk::vcpu::OperandSize::Byte},
        {"w", gnilk::vcpu::OperandSize::Word},
        {"d", gnilk::vcpu::OperandSize::DWord},
        {"l", gnilk::vcpu::OperandSize::Long},
    };

    if (!strToOpSize.contains(opSize.value)) {
        fmt::println(stderr, "Unsupported operand size {}", opSize.value);
        return {false,{}};
    }

    return {true, strToOpSize[opSize.value]};
}


ast::Statement::Ref Parser::ParseDeclaration() {
    Eat();
    auto [haveOpSize, opSize] = ParseOpSize();
    auto array = ast::ArrayLiteral::Create(opSize);

    do {
        auto exp = ParseExpression();
        if (exp->Kind() != ast::NodeType::kNumericLiteral) {
            fmt::println(stderr, "Only numeric literals allowed in array declaration");
            return nullptr;
        }
        array->PushToArray(exp);
        if (At().type != TokenType::Comma) {
            break;
        }
        Eat();
    } while(true);

    return array;
}

ast::Statement::Ref Parser::ParseIdentifierOrInstr() {
    // Check if this is a proper instruction
    if (vcpu::GetOperandFromStr(At().value).has_value()) {
        return ParseInstruction();
    }
    // This is just an identifier - deal with it...
    auto ident = At().value;
    Eat();

    // FIXME: Support constants, check if next is '=' or 'EQU'

    Expect(TokenType::Colon, "Label must end with colon");
    return std::make_shared<ast::Identifier>(ident);
}

ast::Statement::Ref Parser::ParseInstruction() {
    auto operand = At().value;
    auto opClass = gnilk::vcpu::GetOperandFromStr(operand);

    if (!opClass.has_value()) {
        fmt::println(stderr, "Unsupported instruction '{}'", At().value);
        return nullptr;
    }

    auto optionalDesc = vcpu::GetOpDescFromClass(*opClass);

    if (!optionalDesc.has_value()) {
        fmt::println(stderr, "unsupported instruction {}", At().value);
        return nullptr;
    }
    auto opDesc = *optionalDesc;
    if (opDesc.features & vcpu::OperandDescriptionFlags::TwoOperands) {
        return ParseTwoOpInstruction(operand);
    } else if (opDesc.features & vcpu::OperandDescriptionFlags::OneOperand) {
        return ParseOneOpInstruction(operand);
    }

    Eat();      // Consume operand token!

    // We have a single instr. statement..
    return std::make_shared<ast::NoOpInstrStatment>(operand);
}

ast::Statement::Ref Parser::ParseOneOpInstruction(const std::string &symbol) {
    auto instrStatment = std::make_shared<ast::OneOpInstrStatment>(symbol);

    Eat();
    auto [haveOpSize, opSize] = ParseOpSize();

    if (haveOpSize) {
        instrStatment->SetOpSize(opSize);
    }

    auto operand = ParseExpression();
    instrStatment->SetOperand(operand);

    return instrStatment;

}

ast::Statement::Ref Parser::ParseTwoOpInstruction(const std::string &symbol) {
    auto instrStatment = std::make_shared<ast::TwoOpInstrStatment>(symbol);

    Eat();

    auto [haveOpSize, opSize] = ParseOpSize();

    if (haveOpSize) {
        instrStatment->SetOpSize(opSize);
    }

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
        case TokenType::Identifier :
            return ast::Identifier::Create(Eat().value);
        case TokenType::OpenParen :
            // This is a deference-expression...
            Eat();
            auto value = ParseExpression();
            Expect(TokenType::CloseParen, "Expression should end with ')'");
            return ast::DeReferenceExpression::Create(value);
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



