//
// Created by gnilk on 21.12.23.
//

#ifndef STATEMENTS_H
#define STATEMENTS_H

#include "astbase.h"
namespace gnilk {
    namespace ast {
        class MetaStatement : public Statement {
        public:
            using Ref = std::shared_ptr<MetaStatement>;
        public:
            MetaStatement(const std::string &metaSymbol) : Statement(NodeType::kMetaStatement), symbol(metaSymbol) {

            }
            virtual ~MetaStatement() = default;
            void SetOptional(const ast::Statement::Ref &newOptStmtArg) {
                optStmtArg = newOptStmtArg;
            }
            const std::string &Symbol() {
                return symbol;
            }
            ast::Statement::Ref Argument() {
                return optStmtArg;
            }

            void Dump() override {
                WriteLine("MetaStatement");
                Indent();
                WriteLine("Symbol: {}", symbol);
                if (optStmtArg) {
                    WriteLine("Arg");
                    Indent();
                    optStmtArg->Dump();
                    Unindent();
                }
                Unindent();
            }
        protected:
            std::string symbol = {};
            ast::Statement::Ref optStmtArg = {};
        };

        class LineComment : public Statement {
        public:
            using Ref = std::shared_ptr<LineComment>;
        public:
            LineComment() : Statement(NodeType::kCommentStatement) {

            }
            LineComment(const std::string &ctext) : Statement(NodeType::kCommentStatement), text(ctext) {

            }
            virtual ~LineComment() = default;

            const std::string &Text() {
                return text;
            }
            void Dump() override {
                WriteLine("LineComment");
                Indent();
                WriteLine("Text: {}", text);
                Unindent();
            }
        protected:
            std::string text;
        };

        class StructStatement : public Statement {
        public:
            using Ref = std::shared_ptr<StructStatement>;
        public:
            StructStatement() : Statement(NodeType::kStructStatement) {

            }
            StructStatement(const std::string &nameOfStruct, const std::vector<ast::Statement::Ref> &scope) :
                Statement(NodeType::kStructStatement),
                ident(nameOfStruct), declarations(scope) {

            }

            virtual ~StructStatement() = default;

            const std::string &Name() {
                return ident;
            }
            const std::vector<ast::Statement::Ref> &Declarations() {
                return declarations;
            }
            void Dump() override {
                WriteLine("Struct Statement");
                Indent();
                WriteLine("Name: {}", ident);
                WriteLine("Declarations");
                Indent();
                for(auto &d : declarations) {
                    d->Dump();
                }
                Unindent();
                Unindent();
            }

        protected:
            std::string ident;
            std::vector<ast::Statement::Ref> declarations;
        };

        class ReservationStatement : public Statement {
        public:
            using Ref = std::shared_ptr<ReservationStatement>;
        public:
            ReservationStatement() : Statement(NodeType::kReservationStatment) {
            }
            ReservationStatement(ast::Identifier::Ref identifier, vcpu::OperandSize szOperand, ast::Expression::Ref numElem) :
                Statement(NodeType::kReservationStatment),
                ident(identifier), opSize(szOperand), numElemExpression(numElem) {

            }
            virtual ~ReservationStatement() {}

            ast::Identifier::Ref Identifier() {
                return ident;
            }
            vcpu::OperandSize OperandSize() {
                return opSize;
            }
            ast::Expression::Ref NumElements() {
                return numElemExpression;
            }

            void Dump() override {
                WriteLine("Reservation Statement");
                Indent();
                WriteLine("Name: {}", ident->Symbol());
                WriteLine("ElemSize: {}", (int)opSize);
                WriteLine("Elements:");
                Indent();
                numElemExpression->Dump();
                Unindent();
                Unindent();
            }
        protected:
            ast::Identifier::Ref ident;
            vcpu::OperandSize opSize = vcpu::OperandSize::Byte;
            ast::Expression::Ref numElemExpression = {};
        };


    }
}
#endif //STATEMENTS_H
