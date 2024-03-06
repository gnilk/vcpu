//
// Created by gnilk on 14.12.23.
//

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "astbase.h"
#include "InstructionSet.h"

namespace gnilk {
    namespace ast {
        class InstructionStatement : public Statement {
        public:
            using Ref = std::shared_ptr<InstructionStatement>;
        public:
            InstructionStatement() = default;
            explicit InstructionStatement(const std::string &instr) : symbol(instr) {

            }
            const std::string &Symbol() {
                return symbol;
            }
            void Dump() override {
                WriteLine("Instruction");
                Indent();
                WriteLine("Symbol: {}", symbol);
                Unindent();
            }
        protected:
            std::string symbol = {};
        };

        class TwoOpInstrStatment : public Statement {
        public:
            using Ref = std::shared_ptr<TwoOpInstrStatment>;
        public:
            TwoOpInstrStatment() = default;
            explicit TwoOpInstrStatment(const std::string &instr) : Statement(NodeType::kTwoOpInstrStatement), symbol(instr) {

            }
            void SetOpSize(gnilk::vcpu::OperandSize newOpSize) {
                opSize = newOpSize;
            }
            void SetOpFamily(gnilk::vcpu::OperandFamily newOpFamily) {
                opFamily = newOpFamily;
            }
            void SetAddrMode(gnilk::vcpu::AddressMode newAddrMode) {
                addrMode = newAddrMode;
            }
            void SetDst(const Expression::Ref newDst) {
                dst = newDst;
            }
            void SetSrc(const Expression::Ref newSrc) {
                src = newSrc;
            }


            const std::string &Symbol() {
                return symbol;
            }
            gnilk::vcpu::OperandSize OpSize() {
                return opSize;
            }
            gnilk::vcpu::OperandFamily OpFamily() {
                return opFamily;
            }
            gnilk::vcpu::AddressMode AddrMode() {
                return addrMode;
            }

            Expression::Ref Src() {
                return src;
            }
            Expression::Ref Dst() {
                return dst;
            }

            void Dump() override {
                WriteLine("Two Op Instruction");
                Indent();
                WriteLine("Symbol: {}", symbol);
                WriteLine("OpSize: {}", (int)opSize);

                WriteLine("Dest:");
                Indent();
                dst->Dump();
                Unindent();

                WriteLine("Source:");
                Indent();
                src->Dump();
                Unindent();

                Unindent();
            }

        protected:
            std::string symbol = {};
            gnilk::vcpu::AddressMode addrMode = {};
            gnilk::vcpu::OperandSize opSize = gnilk::vcpu::OperandSize::Long;
            gnilk::vcpu::OperandFamily opFamily = gnilk::vcpu::OperandFamily::Integer;
            ast::Expression::Ref dst;
            ast::Expression::Ref src;

        };

        class OneOpInstrStatment : public Statement {
        public:
            using Ref = std::shared_ptr<OneOpInstrStatment>;
        public:
            OneOpInstrStatment() = default;
            explicit OneOpInstrStatment(const std::string &instr) : Statement(NodeType::kOneOpInstrStatement), symbol(instr) {

            }
            void SetOpSize(gnilk::vcpu::OperandSize newOpSize) {
                opSize = newOpSize;
            }
            void SetAddrMode(gnilk::vcpu::AddressMode newAddrMode) {
                addrMode = newAddrMode;
            }
            void SetOperand(const Expression::Ref newOp) {
                operand = newOp;
            }

            const std::string &Symbol() {
                return symbol;
            }
            gnilk::vcpu::OperandSize OpSize() {
                return opSize;
            }
            gnilk::vcpu::AddressMode AddrMode() {
                return addrMode;
            }

            Expression::Ref Operand() {
                return operand;
            }

            void Dump() override {
                WriteLine("One Op Instruction");
                Indent();
                WriteLine("Symbol: {}", symbol);
                WriteLine("OpSize: {}", (int)opSize);

                WriteLine("Operand:");
                Indent();
                operand->Dump();
                Unindent();
                Unindent();
            }

        protected:
            std::string symbol = {};
            gnilk::vcpu::AddressMode addrMode = {};
            gnilk::vcpu::OperandSize opSize = gnilk::vcpu::OperandSize::Long;
            ast::Expression::Ref operand;

        };


        class NoOpInstrStatment : public Statement {
        public:
            using Ref = std::shared_ptr<NoOpInstrStatment>;
        public:
            NoOpInstrStatment() = default;
            explicit NoOpInstrStatment(const std::string &instr) : Statement(NodeType::kNoOpInstrStatement), symbol(instr) {

            }

            const std::string &Symbol() {
                return symbol;
            }

            void Dump() override {
                WriteLine("Instruction");
                Indent();
                WriteLine("Symbol: {}", symbol);
                Unindent();
            }

        protected:
            std::string symbol = {};
        };



    }
}


#endif //INSTRUCTIONS_H
