//
// Created by gnilk on 14.12.23.
//

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "Lexer/TextLexer.h"
#include "ast/ast.h"

namespace gnilk {
    namespace assembler {


        class Parser {
        public:
            Parser() = default;
            virtual ~Parser() = default;

            ast::Program::Ref ProduceCode(const std::string &srcCode);
        protected:
            ast::Program::Ref Begin(const std::string &srcCode);


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

        private:
            Lexer lexer;
            Token last = {};
            std::vector<Token>::iterator it;
            std::vector<Token> tokens;
        };
    }
}



#endif //PARSER_H
