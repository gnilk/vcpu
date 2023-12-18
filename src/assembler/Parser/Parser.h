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
            const Token &Expect(TokenType token, const std::string &errmsg);

            ast::Statement::Ref ParseStatement();
            ast::Statement::Ref ParseDeclaration();
            ast::Statement::Ref ParseIdentifierOrInstr();
            ast::Statement::Ref ParseInstruction();
            ast::Statement::Ref ParseOneOpInstruction(const std::string &symbol);
            ast::Statement::Ref ParseTwoOpInstruction(const std::string &symbol);

            ast::Expression::Ref ParseExpression();
            ast::Expression::Ref ParsePrimaryExpression();

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
