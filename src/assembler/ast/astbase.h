//
// Created by gnilk on 12.12.23.
//

#ifndef ASTBASE_H
#define ASTBASE_H

#include <vector>
#include <string>
#include <memory>

#include "StrUtil.h"
#include "fmt/format.h"

namespace gnilk {

    namespace ast {
        enum class NodeType {
            kUnknown,
            kProgram,
            kScope,

            kVarDeclaration,
            kFunctionDeclaration,

            kInstructionStatement,
            kTwoOpInstrStatement,        // instructions with two operands
            kOneOpInstrStatement,        // instructions with single operand
            kNoOpInstrStatement,         // instructions without operands (nop, brk, etc..)
            kJumpInstrStatement,         // not sure if needed


            kIfStatement,         // Expression or Statement (I think this should be a statement)
            kWhileStatement,      // Expression?
            kBreakStatement,
            kAssignmentStatement,
            kCallStatement,
            kCommentStatement,
            kMetaStatement,
            kStructStatement,
            kReservationStatment,

            kProperty,
            kObjectLiteral,
            kNumericLiteral,
            kStringLiteral,
            kNullLiteral,
            kRegisterLiteral,
            kArrayLiteral,
            kStructLiteral,
            kIdentifier,
            kDeRefExpression,

            kLogicalExpression,
            kUnaryExpression,
            kCompareExpression,
            kBinaryExpression,
            kMemberExpression,
            kCallExpression,
        };

        const std::string &NodeTypeToString(NodeType type);

        class StatementWriter {
        public:
            virtual void Dump() {
                printf("%s", strIndent.c_str());
            }
            const char *StrIndent() {
                return strIndent.c_str();
            }
            void Indent() {
                lvlIndent += 4;
                strIndent = std::string(lvlIndent, ' ');
            }
            void Unindent() {
                lvlIndent -= 4;
                if (lvlIndent < 0) {
                    lvlIndent = 0;
                }
                strIndent = std::string(lvlIndent, ' ');
            }

            template <class...T>
            inline void WriteLine(const std::string& format, T&&... args) {

                fmt::format_arg_store<fmt::format_context, T...> as{args...};
                auto str = fmt::vformat(format, as);

                fprintf(stdout, "%s%s\n", strIndent.c_str(), str.c_str());
            }
            template <class...T>
            inline void Write(const std::string& format, T&&... args) {

                fmt::format_arg_store<fmt::format_context, T...> as{args...};
                auto str = fmt::vformat(format, as);

                fprintf(stdout, "%s%s", strIndent.c_str(), str.c_str());
            }

        protected:
            static int lvlIndent;
            static std::string strIndent;
        };

        class Statement : public StatementWriter {
        public:
            using Ref = std::shared_ptr<Statement>;
        public:
            Statement() = default;
            Statement(NodeType nt) : kind(nt) {

            }
            virtual ~Statement() = default;
            __inline NodeType Kind() const noexcept {
                return kind;
            }
            virtual void Dump() {
                WriteLine("kind={}", (int)kind);
            }
        protected:
            NodeType kind = {};
        };

        class Program : public Statement {
        public:
            using Ref = std::shared_ptr<Program>;
        public:
            Program() : Statement(NodeType::kProgram) {

            };
            virtual ~Program() = default;
            void Append(const Statement::Ref statement) {
                body.push_back(statement);
            }
            const std::vector<Statement::Ref> &Body() const noexcept { return body; }
            void Dump() override {
                WriteLine("Body");
                Indent();
                for(auto &s : body) {
                    s->Dump();
                }
                Unindent();
            }
        protected:
            std::vector<Statement::Ref> body;
        };



        class Expression : public Statement {
        public:
            using Ref = std::shared_ptr<Expression>;
        public:
            Expression() = default;
            explicit Expression(NodeType nt) : Statement(nt) {}
            virtual ~Expression() = default;
        };

        class DeReferenceExpression : public Expression {
        public:
            using Ref = std::shared_ptr<DeReferenceExpression>;
        public:
            DeReferenceExpression() = default;
            explicit DeReferenceExpression(const ast::Expression::Ref exp) : Expression(NodeType::kDeRefExpression), derefexp(exp) {

            }
            virtual ~DeReferenceExpression() = default;

            static Ref Create(const Expression::Ref &exp) {
                return std::make_shared<DeReferenceExpression>(exp);
            }

            void Dump() override {
                WriteLine("Dereference Expression");
                Indent();
                derefexp->Dump();
                Unindent();
            }

            Expression::Ref GetDeRefExp() {
                return derefexp;
            }

        protected:
            Expression::Ref derefexp;
        };



        // This is a literal???
        class Identifier : public Expression {
        public:
            using Ref = std::shared_ptr<Identifier>;
        public:
            Identifier() : Expression(NodeType::kIdentifier) {
            }
            explicit Identifier(const std::string &sym) : Expression(NodeType::kIdentifier), symbol(sym) {

            }
            virtual ~Identifier() = default;

            static Ref Create(const std::string &sym) {
                auto inst = std::make_shared<Identifier>(sym);
                return inst;
            }

            void Dump() override {
                WriteLine("Identifier:  {}", symbol);
            }
            const std::string &Symbol() const {
                return symbol;
            }
        protected:
            std::string symbol;
        };




        // This I am not sure about - part of an object
        class Property : public Expression {
        public:
            using Ref = std::shared_ptr<Property>;
        public:
            Property() : Expression(NodeType::kProperty) {

            }
            Property(const std::string &newKey) :
                Expression(NodeType::kProperty), key(newKey) {

            }

            Property(const std::string &newKey, const Expression::Ref &newValue) :
                Expression(NodeType::kProperty), key(newKey), value(newValue) {

            }

            static Ref Create(const std::string &newKey) {
                auto inst = std::make_shared<Property>(newKey);
                return inst;
            }

            static Ref Create(const std::string &newKey, const Expression::Ref &newValue) {
                auto inst = std::make_shared<Property>(newKey, newValue);
                return inst;
            }

            virtual ~Property() = default;

            const std::string &Key() const noexcept {
                return key;
            }

            const Expression::Ref &Value() const noexcept {
                return value;
            }

            bool HasValue() const noexcept {
                return (value != nullptr);
            }

            void Dump() override {
                if (value == nullptr) {
                    WriteLine("Key={} (value is null)", key);
                    return;
                }
                WriteLine("Key={}", key);
                value->Dump();
            }

        protected:
            std::string key = {};
            Expression::Ref value = nullptr;        // this is allowed to be null 'unassigned'
        };
    }

}


#endif //ASTBASE_H
