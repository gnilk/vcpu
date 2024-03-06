//
// Created by gnilk on 12.12.23.
//

#ifndef GNILK_AST_LITERALS_H
#define GNILK_AST_LITERALS_H

#include "astbase.h"


namespace gnilk {
    namespace ast {
        class Literal : public Expression {
        public:
            using Ref = std::shared_ptr<Literal>;
        public:
            Literal() = default;
            explicit Literal(NodeType nt) : Expression(nt) {}
            virtual ~Literal() = default;

        };

        class ConstLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<ConstLiteral>;
        public:
            ConstLiteral() : Literal(NodeType::kConstLiteral) {

            }
            explicit ConstLiteral(const std::string &symbol, ast::Expression::Ref exp) :
                Literal(NodeType::kConstLiteral), ident(symbol), constExpression(exp) {

            }
            const std::string &Ident() {
                return ident;
            }
            const ast::Expression::Ref Expression() {
                return constExpression;
            }
            void Dump() override {
                WriteLine("ConstExpression");
                Indent();
                WriteLine("Ident: {}", ident);
                constExpression->Dump();
                Unindent();
            }
        private:
            std::string ident;
            ast::Expression::Ref constExpression = nullptr;
        };

        class NumericLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<NumericLiteral>;
        public:
            NumericLiteral() : Literal(NodeType::kNumericLiteral) {

            }
            explicit NumericLiteral(int32_t num) : Literal(NodeType::kNumericLiteral), number(num) {

            }
            virtual ~NumericLiteral() = default;

            static Ref Create(const std::string &strValue) {
                auto inst = std::make_shared<NumericLiteral>(strutil::to_int32(strValue));
                return inst;
            }

            static Ref Create(int32_t intValue) {
                auto inst = std::make_shared<NumericLiteral>(intValue);
                return inst;
            }

            static Ref CreateFromHex(const std::string &strValue) {
                auto inst = std::make_shared<NumericLiteral>(strutil::hex2dec(strValue));
                return inst;
            }

            void Dump() override {
                WriteLine("Numeric: {}", number);
            }
            int32_t Value() const noexcept {
                return number;
            }
        protected:
            int32_t number; // FIXME: this should be 'Number' union which holds all possible number types...
        };

        class StringLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<StringLiteral>;
        public:
            StringLiteral() : Literal(NodeType::kStringLiteral) {

            }
            virtual ~StringLiteral() = default;
            explicit StringLiteral(const std::string &str) : Literal(NodeType::kStringLiteral), value(str) {

            }
            static Ref Create(const std::string &str) {
                auto inst = std::make_shared<StringLiteral>(str);
                return inst;
            }
            void Dump() override {
                WriteLine("String:  {}", value);
            }

            const std::string &Value() const noexcept {
                return value;
            }

        protected:
            std::string value = {};
        };

        class NullLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<NullLiteral>;
        public:
            NullLiteral() : Literal(NodeType::kNullLiteral) {

            }
            virtual ~NullLiteral() = default;

            static Ref Create() {
                return std::make_shared<NullLiteral>();
            }

            void Dump() override {
                WriteLine("Null");
            }
        };

        class ArrayLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<ArrayLiteral>;
        public:
            ArrayLiteral() : Literal(NodeType::kArrayLiteral) {}
            ArrayLiteral(vcpu::OperandSize szElem) : Literal(NodeType::kArrayLiteral), opSize(szElem) {

            }
            virtual ~ArrayLiteral() {

            }

            static Ref Create(vcpu::OperandSize szElem) {
                return std::make_shared<ArrayLiteral>(szElem);
            }
            // Currently we only support numeric literal expression but let's assume we want more (like StringLiterals, and others)
            void PushToArray(Expression::Ref exp) {
                array.push_back(exp);
            }

            gnilk::vcpu::OperandSize OpSize() {
                return opSize;
            }

            std::vector<Expression::Ref> &Array() {
                return array;
            }

            void Dump() override {
                WriteLine("ArrayLiteral");
                Indent();
                WriteLine("ElemSize: {}", (int)opSize);
                WriteLine("Elements:");
                Indent();
                for(auto &n : array) {
                    n->Dump();
                }
                Unindent();
                Unindent();
            }

        protected:
            vcpu::OperandSize opSize = vcpu::OperandSize::Byte;
            std::vector<Expression::Ref> array;
        };


        class StructLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<StructLiteral>;
        public:
            StructLiteral() : Literal(NodeType::kStructLiteral) {}
            StructLiteral(const std::string &typeStruct) : Literal(NodeType::kStructLiteral),
                structTypeName(typeStruct) {}

            virtual ~StructLiteral() = default;

            const std::string &StructTypeName() {
                return structTypeName;
            }

            const std::vector<ast::Statement::Ref> &Members() {
                return members;
            }
            void AddMember(ast::Statement::Ref newMember) {
                members.push_back(newMember);
            }

            void Dump() override {
                WriteLine("Struct Literal");
                Indent();
                WriteLine("Struct Type: {}", structTypeName);
                WriteLine("Members:");
                Indent();
                for(auto &m : members) {
                    m->Dump();
                }
                Unindent();
                Unindent();
            }
        protected:
            std::string structTypeName;
            std::vector<ast::Statement::Ref> members;
        };


        class RegisterLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<RegisterLiteral>;
        public:
            RegisterLiteral() : Literal(NodeType::kRegisterLiteral) {

            }

            explicit RegisterLiteral(const std::string &reg) : Literal(NodeType::kRegisterLiteral), regValue(reg) {

            }

            virtual ~RegisterLiteral() = default;

            static Ref Create(const std::string &reg) {
                return std::make_shared<RegisterLiteral>(reg);
            }

            const std::string &Symbol() {
                return regValue;
            }

            void Dump() override {
                WriteLine("Reg: {}", regValue);
            }

        protected:
            std::string regValue = {};
        };

        class RelativeRegisterLiteral : public Literal {
        public:
            using Ref = std::shared_ptr<RelativeRegisterLiteral>;
        public:
            RelativeRegisterLiteral() : Literal(NodeType::kRelativeRegisterLiteral) {

            }

            explicit RelativeRegisterLiteral(const ast::RegisterLiteral::Ref &baseReg, const ast::Expression::Ref &relative) :
                Literal(NodeType::kRelativeRegisterLiteral),
                baseRegValue(baseReg), relValue(relative) {

            }

            virtual ~RelativeRegisterLiteral() = default;

            static Ref Create(const ast::RegisterLiteral::Ref  &baseReg, const ast::Expression::Ref &relReg) {
                return std::make_shared<RelativeRegisterLiteral>(baseReg, relReg);
            }

            void SetOperator(const std::string &newOpr) {
                opr = newOpr;
            }

            const std::string &Operator() {
                return opr;
            }

            ast::RegisterLiteral::Ref BaseRegister() {
                return baseRegValue;
            }
            ast::Expression::Ref RelativeExpression() {
                return relValue;
            }

            void Dump() override {
                WriteLine("Relative Register Literal");
                Indent();
                WriteLine("Base Reg: {}", baseRegValue->Symbol());
                WriteLine("Relative");
                Indent();
                relValue->Dump();
                Unindent();

            }

        protected:
            ast::RegisterLiteral::Ref baseRegValue = {};
            ast::Expression::Ref relValue = {};
            // empty by default - compiler will check applicable
            std::string opr = {};
        };
    }    // namespace ast
}    // namespace gnilk

#endif //LITERALS_H
