//
// Created by gnilk on 21.12.23.
//

#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

namespace gnilk {
    namespace ast {
        class BinaryExpression : public Expression {
        public:
            using Ref = std::shared_ptr<BinaryExpression>;
        public:
            BinaryExpression() : Expression(NodeType::kBinaryExpression) {}
            BinaryExpression(const Expression::Ref newLeft, const Expression::Ref newRight, const std::string &oper) :
                Expression(NodeType::kBinaryExpression), left(newLeft), right(newRight), opr(oper) {}

            virtual ~BinaryExpression() = default;
            void Dump() override {
                WriteLine("Binary Expr");
                Indent();
                WriteLine("Oper: {}", opr);
                WriteLine("Left:");
                Indent();
                left->Dump();
                Unindent();
                WriteLine("Right:");
                Indent();
                right->Dump();
                Unindent();
                Unindent();
            }
            const Expression::Ref &Left() const noexcept {
                return left;
            }
            const Expression::Ref &Right() const noexcept {
                return right;
            }
            const std::string &Operator() const noexcept {
                return opr;
            }
        protected:
            Expression::Ref left = {};
            Expression::Ref right = {};
            std::string opr = {};
        };


        class UnaryExpression : public Expression {
        public:
            using Ref = std::shared_ptr<UnaryExpression>;
        public:
            UnaryExpression() = default;
            UnaryExpression(const Expression::Ref &rhs, const std::string &op) :
                Expression(NodeType::kUnaryExpression), right(rhs), oper(op) {

            }
            virtual ~UnaryExpression() = default;

            void Dump() override {
                WriteLine("Unary Expr");
                Indent();
                WriteLine("Op: {}", oper);
                WriteLine("Right");
                Indent();
                right->Dump();
                Unindent();
                Unindent();
            }

        protected:
            std::string oper = {};
            Expression::Ref right = {};
        };

    }
}

#endif //EXPRESSIONS_H
