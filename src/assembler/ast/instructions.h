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
                WriteLine("Instruction");
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
            ast::Expression::Ref dst;
            ast::Expression::Ref src;

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
