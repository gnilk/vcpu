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
            using Ref = std::shared_ptr<InstructionStatement>;
        public:
            MoveInstrStatment() = default;
            explicit MoveInstrStatment(const std::string &instr) : Statement(NodeType::kMoveInstrStatement), symbol(instr) {

            }
            void SetOpSize(OperandSize newOpSize) {
                opSize = newOpSize;
            }
            void SetDst(const std::string &newDst) {
                dst = newDst;
            }
            void SetSrc(const std::string &newSrc) {
                src = newSrc;
            }
            void Dump() override {
                WriteLine("Instruction");
                Indent();
                WriteLine("Symbol: {}", symbol);
                WriteLine("OpSize: {}", (int)opSize);
                WriteLine("Dest..: {}", dst);
                WriteLine("Source: {}", src);

                Unindent();
            }

        protected:
            std::string symbol = {};
            AddressMode adrMode = {};
            OperandSize opSize = OperandSize::Long;
            std::string dst = {};
            std::string src = {};

        };
    }
}


#endif //INSTRUCTIONS_H
