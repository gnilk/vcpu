//
// Created by gnilk on 14.12.23.
//

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "astbase.h"
#include "../../vcpu/VirtualCPU.h"

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

        class MoveInstrStatment : public Statement {
        public:
            using Ref = std::shared_ptr<MoveInstrStatment>;
        public:
            MoveInstrStatment() = default;
            explicit MoveInstrStatment(const std::string &instr) : Statement(NodeType::kMoveInstrStatement), symbol(instr) {

            }
            void SetOpSize(OperandSize newOpSize) {
                opSize = newOpSize;
            }
            void SetAddrMode(AddressMode newAddrMode) {
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
            OperandSize OpSize() {
                return opSize;
            }
            AddressMode AddrMode() {
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
            AddressMode addrMode = {};
            OperandSize opSize = OperandSize::Long;
            ast::Expression::Ref dst;
            ast::Expression::Ref src;

        };
    }
}


#endif //INSTRUCTIONS_H
