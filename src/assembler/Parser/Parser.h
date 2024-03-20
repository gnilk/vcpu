//
// Created by gnilk on 14.12.23.
//

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <utility>
#include "Lexer/TextLexer.h"
#include "ast/ast.h"

namespace gnilk {
    namespace assembler {


        class Parser {
        public:
            Parser() = default;
            virtual ~Parser() = default;

            ast::Program::Ref ProduceAST(const std::string_view &srcCode);
        protected:
            ast::Program::Ref Begin(const std::string_view &srcCode);


            __inline bool Done() {
                return (At().type == TokenType::EoF || it == tokens.end());
            }
            __inline const Token &At() {
                return *it;
            }
            __inline const Token &Eat() {
                auto &current= At();
                last = current;
                ++it;
                return current;
            }
            // TODO: Rewrite expect as ast::StatementWriter::WriteLine, so we can supply arguments!
            const Token &Expect(TokenType token, const std::string &errmsg);

            ast::Statement::Ref ParseStatement();

            ast::Statement::Ref ParseConst();
            ast::Statement::Ref ParseStruct();
            ast::Statement::Ref ParseExport();
            ast::Statement::Ref ParseReservationStatement();
            ast::Statement::Ref ParseMetaStatement();
            ast::Statement::Ref ParseLineComment();
            ast::Statement::Ref ParseDeclaration();
            ast::Statement::Ref ParseArrayDeclaration(vcpu::OperandSize opSize);
            ast::Statement::Ref ParseStructDeclaration();
            ast::Statement::Ref ParseIdentifierOrInstr();
            ast::Statement::Ref ParseInstruction();
            ast::Statement::Ref ParseOneOpInstruction(const std::string &symbol);
            ast::Statement::Ref ParseTwoOpInstruction(const std::string &symbol);
            ast::Expression::Ref ParseExpression();
            ast::Expression::Ref ParseAdditiveExpression();
            ast::Expression::Ref ParseMultiplicativeExpression();
            ast::Expression::Ref ParseUnaryExpression();
            ast::Expression::Ref ParsePrimaryExpression();

            union OpSizeOrStruct {
                vcpu::OperandSize opSize;
                bool structType;
            };

            enum class ParseOpSizeResult {
                Error,
                OpSize,
                StructType,
            };

            std::pair<ParseOpSizeResult, OpSizeOrStruct> ParseOpSizeOrStruct();
            std::pair<bool, vcpu::OperandSize> ParseOpSize();
            //std::optional<vcpu::OperandSize> ParseOpSize();

        private:
            Lexer lexer;
            Token last = {};
            std::vector<Token>::iterator it;
            std::vector<Token> tokens;
        };
    }
}



#endif //PARSER_H
