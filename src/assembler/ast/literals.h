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

        class NullLiteral : public Expression {
        public:
            using Ref = std::shared_ptr<NullLiteral>;
        public:
            NullLiteral() : Expression(NodeType::kNullLiteral) {

            }
            virtual ~NullLiteral() = default;

            static Ref Create() {
                return std::make_shared<NullLiteral>();
            }

            void Dump() override {
                WriteLine("Null");
            }
        };

        class ArrayLiteral : public Expression {
        public:
            using Ref = std::shared_ptr<ArrayLiteral>;
        public:
            ArrayLiteral() : Expression(NodeType::kArrayLiteral) {}
            ArrayLiteral(vcpu::OperandSize szElem) : Expression(NodeType::kArrayLiteral), opSize(szElem) {

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

        class RegisterLiteral : public Expression {
        public:
            using Ref = std::shared_ptr<RegisterLiteral>;
        public:
            RegisterLiteral() : Expression(NodeType::kRegisterLiteral) {

            }

            explicit RegisterLiteral(const std::string &reg) : Expression(NodeType::kRegisterLiteral), regValue(reg) {

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




    }    // namespace ast
}    // namespace gnilk

#endif //LITERALS_H
