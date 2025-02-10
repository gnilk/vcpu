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

        class ExportStatement : public Statement {
        public:
            using Ref = std::shared_ptr<ExportStatement>;
        public:
            ExportStatement() : Statement(NodeType::kExportStatement) {

            }

            explicit ExportStatement(const std::string &identifier) :
                Statement(NodeType::kExportStatement),
                ident(identifier) {

            }

            const std::string &Identifier() {
                return ident;
            }

            void Dump() override {
                WriteLine("Export/Public Statement");
                Indent();
                WriteLine("Identifier: {}", ident);
                Unindent();
                Unindent();
            }
        protected:
            std::string ident;
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

            size_t SizeOf() override {
                size_t sz = 0;
                for(auto &d : declarations) {
                    sz += SizeOf();
                }
                return sz;
            }

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

        // FIXME: Consider splitting this into two statement
        //        One for NativeOperand size and one for custom types
        //        This one can still be the base class for such
        class ReservationStatement : public Statement {
        public:
            using Ref = std::shared_ptr<ReservationStatement>;
        public:
            ReservationStatement() = delete;
            ReservationStatement(NodeType kind) : Statement(kind) {
            }
            ReservationStatement(NodeType subKind,ast::Identifier::Ref identifier, ast::Expression::Ref numElem) :
                    Statement(subKind),
                    ident(identifier), numElemExpression(numElem) {

            }
            virtual ~ReservationStatement() {}

            ast::Identifier::Ref Identifier() {
                return ident;
            }

            size_t SizeOf() override {
                return 0;
            }


            ast::Expression::Ref NumElements() {
                return numElemExpression;
            }

            void Dump() override {
                WriteLine("Name: {}", ident->Symbol());
                WriteLine("Elements:");
                Indent();
                numElemExpression->Dump();
                Unindent();
            }
        protected:
            ast::Identifier::Ref ident = {};
            ast::Expression::Ref numElemExpression = {};
        };

        class ReservationStatementNativeType : public ReservationStatement {
        public:
            using Ref = std::shared_ptr<ReservationStatementNativeType>;
        public:
            ReservationStatementNativeType() : ReservationStatement(NodeType::kReservationStatementNative) {
            }
            ReservationStatementNativeType(ast::Identifier::Ref identifier, vcpu::OperandSize szOperand, ast::Expression::Ref numElem) :
                ReservationStatement(NodeType::kReservationStatementNative, identifier, numElem), nativeType(szOperand) {

            }

            size_t ElementByteSize() {
                switch(nativeType) {
                    case vcpu::OperandSize::Byte :
                        return 1;
                    case vcpu::OperandSize::Word :
                        return 2;
                    case vcpu::OperandSize::DWord :
                        return 4;
                    case vcpu::OperandSize::Long :
                        return 8;
                }
                return 0;
            }

            void Dump() override {
                WriteLine("Reservation Statement Native Type");
                Indent();
                ReservationStatement::Dump();
                WriteLine("Elem.ByteSize: {}", ElementByteSize());
                Unindent();
            }
        protected:
            vcpu::OperandSize nativeType;

        };
        class ReservationStatementCustomType : public ReservationStatement {
        public:
            using Ref = std::shared_ptr<ReservationStatementCustomType>;
        public:
            ReservationStatementCustomType() : ReservationStatement(NodeType::kReservationStatementCustom) {
            }
            ReservationStatementCustomType(ast::Identifier::Ref identifier, ast::StructStatement::Ref typeStmt, ast::Expression::Ref numElem) :
                ReservationStatement(NodeType::kReservationStatementCustom, identifier, numElem), customType(typeStmt) {
            }

            void Dump() override {
                WriteLine("Reservation Statement Custom Type");
                Indent();
                ReservationStatement::Dump();
                // struct size - needed?
                WriteLine("StructSize: {}", 0);
                customType->Dump();

            }
            ast::Statement::Ref CustomType() {
                return customType;
            }
        protected:
            ast::StructStatement::Ref customType;
        };


    }
}
#endif //STATEMENTS_H
