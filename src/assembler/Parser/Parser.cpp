//
// Created by gnilk on 14.12.23.
//

#include <map>
#include <unordered_set>
#include "Parser.h"
#include "Preprocessor/PreProcessor.h"
#include "InstructionSetV1/InstructionSetV1Def.h"
#include "InstructionSet.h"
#include "ast/statements.h"
/*
 * TO-OD:
 * - Move over Expression and Literals from 'astparser' project
 * - Add 'register-literal' to Literals
 * - Add more or less all AST handling from Expression and onwards; this enables: "move d0, <expression>"
 */


using namespace gnilk;
using namespace gnilk::assembler;

ast::Program::Ref Parser::ProduceAST(const std::string_view &srcCode) {
    ast::Statement::Ref stmtPrevious = nullptr;
    auto program = Begin(srcCode);
    if (!program) {
        return nullptr;
    }

    while(!Done()) {
        // EOL statements - we just continue
        // if (At().type == TokenType::EoL) {
        //     Eat();
        //     continue;
        // }
        auto stmt = ParseStatement();

        // we could expect ';' here?
        if (stmt == nullptr) {
            return nullptr;
        }
        program->Append(stmt);
        stmtPrevious = stmt;
    }
    return program;
}

ast::Program::Ref Parser::Begin(const std::string_view &srcCode) {
    PreProcessor preProcessor;

    tokens = lexer.Tokenize(srcCode);

    if (assetLoader != nullptr) {
        if (preProcessor.Process(tokens, assetLoader) == false) {
            return nullptr;
        }
    }

    it = tokens.begin();
    return std::make_shared<ast::Program>();
}

ast::Statement::Ref Parser::ParseStatement() {
    switch(At().type) {
        case TokenType::Dot :
            return ParseMetaStatement();
        case TokenType::MetaKeyword :       // Meta keyword should ALWAYS be proceeded with a dot - if not, this is not a meta
        case TokenType::Identifier :
            return ParseIdentifierOrInstr();
        case TokenType::Declaration :
            return ParseDeclaration();
        case TokenType::Reservation :
            return ParseReservationStatement();
        case TokenType::LineComment :
            return ParseLineComment();
        case TokenType::Public :
            return ParseExport();
        case TokenType::Struct :
            return ParseStructDefinition();
        case TokenType::Const :
            return ParseConst();
        default:
            break;
    }
    return nullptr;
}

ast::Statement::Ref Parser::ParseConst() {
    Eat();
    if (At().type != TokenType::Identifier) {
        fmt::println(stderr, "Parser, Identifier expected after const; const <id>, got: {}", At().value);
        return nullptr;
    }
    auto ident = Eat().value;
    auto expression = ParseExpression();

    return std::make_shared<ast::ConstLiteral>(ident, expression);
}

ast::Statement::Ref Parser::ParseStructDefinition() {
    Eat();
    if (At().type != TokenType::Identifier) {
        fmt::println(stderr, "Parser, Identifier expected for struct; struct <name>,  got: {}", At().value);
        return nullptr;
    }
    auto ident = Eat().value;
    Expect(TokenType::OpenBrace, "Parser, missing '{' at beginning of struct declaration");

    std::vector<ast::Statement::Ref> declarations;
    // This is one of the few spawning multiple lines...
    while(At().type != TokenType::CloseBrace) {
        // if (At().type == TokenType::EoL) {
        //     Eat();
        //     continue;
        // }


        auto stmt = ParseReservationStatement();
        if (stmt == nullptr) {
            return nullptr;
        }
        declarations.push_back(stmt);
    }
    Eat();

    auto structStmt = std::make_shared<ast::StructStatement>(ident, declarations);

    typedefCache[ident] = structStmt;

    return structStmt;
}

ast::Statement::Ref Parser::ParseReservationStatement() {
    auto identifier = ParseExpression();
    if (identifier->Kind() != ast::NodeType::kIdentifier) {
        fmt::println(stderr, "Parser, Expected Identifier in struct body!");
        return nullptr;
    }

    Expect(TokenType::Declaration, "'dc' must follow identifier!");

    auto [parseRes, opSize] = ParseOpSizeOrStruct();
    if (parseRes == ParseOpSizeResult::Error) {
        return nullptr;
    }

    if (parseRes == ParseOpSizeResult::StructType) {
        return ParseCustomTypeReservationStatement(identifier);
    }
    return ParseNativeReservationStatement(identifier, opSize);
}

ast::Statement::Ref Parser::ParseNativeReservationStatement(ast::Expression::Ref identifier, OpSizeOrStruct opSize) {

    auto numToReserve = ParseExpression();
    if (numToReserve->Kind() != ast::NodeType::kNumericLiteral) {
        fmt::println(stderr, "Parser, expected numeric literal for number of elements");
        return nullptr;
    }

    return std::make_shared<ast::ReservationStatementNativeType>(std::dynamic_pointer_cast<ast::Identifier>(identifier), opSize.opSize, numToReserve);
}

ast::Statement::Ref Parser::ParseCustomTypeReservationStatement(ast::Expression::Ref identifier) {
    // FIXME: Here we should parse a CustomTypeReservationStatment

    // Could use something else - like ParseStatement - this would accept a whole new range of things; like struct def in struct def
    if (At().type != TokenType::Identifier) {
        fmt::println(stderr, "Parser, identifier must follow struct");
        return nullptr;
    }

    if (!typedefCache.contains(At().value)) {
        fmt::println(stderr, "Parser, invalid identifier '{}' - unknown type", At().value);
        return nullptr;
    }
    auto structDef = typedefCache[At().value];
    if (structDef->Kind() != ast::NodeType::kStructStatement) {
        fmt::println(stderr, "Parser, reservation type is not struct");
        return nullptr;
    }
    Eat();

    ast::Expression::Ref numToReserve;
    if (At().type != TokenType::Comma) {
        numToReserve = std::make_shared<ast::NumericLiteral>(1);
    } else {
        Eat();
        numToReserve = ParseExpression();
    }
    return std::make_shared<ast::ReservationStatementCustomType>(std::dynamic_pointer_cast<ast::Identifier>(identifier),
                                                       std::dynamic_pointer_cast<ast::StructStatement>(structDef), numToReserve);
}

ast::Statement::Ref Parser::ParseExport() {
    Eat();
    if (At().type != TokenType::Identifier) {
        fmt::println(stderr, "Parser, Identifier expected after public or export, got {}", At().value);
        return nullptr;
    }
    auto &identifier = At().value;
    auto stmt = std::make_shared<ast::ExportStatement>(identifier);
    Eat();

    return stmt;
}


ast::Statement::Ref Parser::ParseMetaStatement() {
    Eat();
    if (At().type != TokenType::MetaKeyword) {
        fmt::println(stderr,"Parser, Meta Keyword expected in meta statement; .<meta>, got: {}", At().value);
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
    //Expect(TokenType::EoL, "Should be end of line");
    return std::make_shared<ast::LineComment>(text.value);
}

std::pair<bool, vcpu::OperandSize> Parser::ParseOpSize() {
    if (At().type != TokenType::Dot) {
        return {false,{}};
    }
    Eat();  // Remove dot

    // take op size declaration
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
std::pair<Parser::ParseOpSizeResult, Parser::OpSizeOrStruct> Parser::ParseOpSizeOrStruct() {
    if (At().type != TokenType::Dot) {
        return {ParseOpSizeResult::Error,{}};
    }
    Eat();

    auto opSize = Eat();
    if (opSize.value == "struct") {
        return {ParseOpSizeResult::StructType, {}};
    }

    static std::unordered_map<std::string, gnilk::vcpu::OperandSize> strToOpSize = {
        {"b", gnilk::vcpu::OperandSize::Byte},
        {"w", gnilk::vcpu::OperandSize::Word},
        {"d", gnilk::vcpu::OperandSize::DWord},
        {"l", gnilk::vcpu::OperandSize::Long},
    };

    if (!strToOpSize.contains(opSize.value)) {
        fmt::println(stderr, "Unsupported operand size {}", opSize.value);
        return {ParseOpSizeResult::Error,{}};
    }

    return {ParseOpSizeResult::OpSize, {.opSize = strToOpSize[opSize.value]}};
}

ast::Statement::Ref Parser::ParseDeclaration() {
    Eat();
    auto [result, opSizeOrStruct] = ParseOpSizeOrStruct();

    if (result == ParseOpSizeResult::OpSize) {
        return ParseArrayDeclaration(opSizeOrStruct.opSize);
    }
    if (result == ParseOpSizeResult::StructType) {
        return ParseStructDeclaration();
    }
    return nullptr;
}

ast::Statement::Ref Parser::ParseStructDeclaration() {
    if (At().type != TokenType::Identifier) {
        fmt::println(stderr, "Parser, Identifer of struct expected after 'dc.struct'");
        return nullptr;
    }
    auto identifier = Eat();
    Expect(TokenType::OpenBrace, "Parser, missing '{' after identifier for struct instance");

    auto structDecl = std::make_shared<ast::StructLiteral>(identifier.value);

    while(At().type != TokenType::CloseBrace) {
        // if (At().type == TokenType::EoL) {
        //     Eat();
        //     continue;
        // }

        auto declStmt = ParseStatement();

        if (declStmt == nullptr) {
            return nullptr;
        }

        if (declStmt->Kind() == ast::NodeType::kArrayLiteral) {
            structDecl->AddMember(declStmt);
        }
    }
    Eat();  // Close brace
    return structDecl;
}

ast::Statement::Ref Parser::ParseArrayDeclaration(vcpu::OperandSize opSize) {
    auto array = ast::ArrayLiteral::Create(opSize);

    do {
        auto exp = ParseExpression();

        if ((exp->Kind() == ast::NodeType::kNumericLiteral) || (exp->Kind() == ast::NodeType::kStringLiteral)) {
            array->PushToArray(exp);
            if (At().type != TokenType::Comma) {
                break;
            }
            Eat();
        } else {
            fmt::println(stderr, "Only numeric literals allowed in array declaration");
            return nullptr;
        }
    } while(true);

    return array;
}

ast::Statement::Ref Parser::ParseIdentifierOrInstr() {
    // Check if this is a proper instruction
    auto &definition = vcpu::InstructionSetManager::Instance().GetInstructionSet().GetDefinition();
    if (definition.GetOperandFromStr(At().value).has_value()) {
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
    auto &instructionSet = vcpu::InstructionSetManager::Instance().GetInstructionSet();

    auto operand = At().value;
    auto opClass = instructionSet.GetDefinition().GetOperandFromStr(operand);

    if (!opClass.has_value()) {
        fmt::println(stderr, "Unsupported instruction '{}'", At().value);
        return nullptr;
    }

    //
    // FIXME: handle extensions!
    //
    auto optionalDesc = instructionSet.GetDefinition().GetOpDescFromClass(*opClass);

    if (!optionalDesc.has_value()) {
        fmt::println(stderr, "unsupported instruction {}", At().value);
        return nullptr;
    }
    auto opDesc = *optionalDesc;
    // FIXME: consolidate the op-desc-flags
    if (opDesc.features & vcpu::OperandFeatureFlags::kFeature_TwoOperands) {
        return ParseTwoOpInstruction(operand);
    } else if (opDesc.features & vcpu::OperandFeatureFlags::kFeature_OneOperand) {
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
    instrStatment->SetDst(dst);

    if (dst->Kind() == ast::NodeType::kRegisterLiteral) {
        auto dstReg = std::dynamic_pointer_cast<ast::RegisterLiteral>(dst);
        if (strutil::startsWith(dstReg->Symbol(), "cr")) {
            instrStatment->SetOpFamily(gnilk::vcpu::OperandFamily::Control);
        }
    }

    // literal must be separated by ','
    Expect(TokenType::Comma, "Operand requires two arguments!");


    // Fixme: replace with: src = ParseExpression();
    auto src = ParseExpression();
    instrStatment->SetSrc(src);
    if (src->Kind() == ast::NodeType::kRegisterLiteral) {
        auto srcReg = std::dynamic_pointer_cast<ast::RegisterLiteral>(src);
        if (strutil::startsWith(srcReg->Symbol(), "cr")) {
            instrStatment->SetOpFamily(gnilk::vcpu::OperandFamily::Control);
        }
    }


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
    return ParseAdditiveExpression();
}

ast::Expression::Ref Parser::ParseAdditiveExpression() {
    auto left = ParseMultiplicativeExpression();
    while(At().value == "+" || At().value == "-") {
        auto oper = Eat().value;
        auto right = ParseMultiplicativeExpression();
        left = std::make_shared<ast::BinaryExpression>(left,right,oper);
    }
    return left;
}

ast::Expression::Ref Parser::ParseMultiplicativeExpression() {
    auto left = ParseUnaryExpression();
    while(At().value == "/" || At().value == "*" || At().value == "%" || (At().value == "<<") || (At().value == ">>")) {
        auto oper = Eat().value;
        auto right = ParseUnaryExpression();
        left = std::make_shared<ast::BinaryExpression>(left, right, oper);
    }
    return left;
}

ast::Expression::Ref Parser::ParseUnaryExpression() {
    if ((At().type == TokenType::Exclamation) || (At().value == "-")) {
        auto oper = Eat().value;
        auto right = ParsePrimaryExpression();
        return std::make_shared<ast::UnaryExpression>(right, oper);
    }
    return ParsePrimaryExpression();
}

ast::Expression::Ref Parser::ParsePrimaryExpression() {
    auto tokenType = At().type;
    switch(tokenType) {
        case TokenType::DataReg :
            return ast::RegisterLiteral::Create(Eat().value);
        case TokenType::AddressReg :
            return ast::RegisterLiteral::Create(Eat().value);
        case TokenType::ControlReg :
            return ast::RegisterLiteral::Create(Eat().value);
        case TokenType::Number :
            return ast::NumericLiteral::Create(Eat().value);
        case TokenType::NumberHex :
            return ast::NumericLiteral::CreateFromHex(Eat().value);
        case TokenType::String :
            return ast::StringLiteral::Create(Eat().value);
        case TokenType::Identifier : {
                auto ident = ast::Identifier::Create(Eat().value);
                // Make sure next after dot is another identifier so we don't have meta statement...
                if ((At().type == TokenType::Dot) && (Peek().type == TokenType::Identifier)) {
                    Eat();
                    auto member = ParsePrimaryExpression();
                    return ast::MemberExpression::Create(ident, member);
                }
                return ident;
            }
            //return ast::Identifier::Create(Eat().value);
        case TokenType::OpenParen :
            // This is a deference-expression...
            {
                Eat();
                auto value = ParseExpression();
                Expect(TokenType::CloseParen, "Expression should end with ')'");
                return ast::DeReferenceExpression::Create(value);
            }
        default:
            break;
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



