//
// Created by gnilk on 15.12.23.
//

#ifndef COMPILER_H
#define COMPILER_H

#include "InstructionSet.h"
#include "ast/ast.h"
#include "Linker/CompiledUnit.h"
#include "IdentifierRelocatation.h"

namespace gnilk {
    namespace assembler {

        class Compiler {
        public:
            Compiler() = default;
            virtual ~Compiler() = default;

            bool CompileAndLink(gnilk::ast::Program::Ref program);
            bool Compile(gnilk::ast::Program::Ref program);
            bool Link();

            // TEMP TEMP
            const std::vector<uint8_t> &Data() {
                return unit.Data();
            }


        protected:
            bool ProcessStmt(ast::Statement::Ref stmt);
            bool ProcessIdentifier(ast::Identifier::Ref identifier);
            bool ProcessNoOpInstrStmt(ast::NoOpInstrStatment::Ref stmt);
            bool ProcessOneOpInstrStmt(ast::OneOpInstrStatment::Ref stmt);
            bool ProcessTwoOpInstrStmt(ast::TwoOpInstrStatment::Ref stmt);

            bool ProcessArrayLiteral(ast::ArrayLiteral::Ref stmt);
            bool ProcessStructLiteral(ast::StructLiteral::Ref stmt);
            bool ProcessConstLiteral(ast::ConstLiteral::Ref stmt);

            bool ProcessMetaStatement(ast::MetaStatement::Ref stmt);
            bool ProcessStructStatement(ast::StructStatement::Ref stmt);


            // FIXME: Replace literal with CompilerValue (or similar) - like 'RuntimeValue' from Interpreter
            ast::Literal::Ref EvaluateConstantExpression(ast::Expression::Ref expression);
            ast::Literal::Ref EvaluateBinaryExpression(ast::BinaryExpression::Ref expression);

            bool EmitOpCodeForSymbol(const std::string &symbol);
            bool EmitInstrOperand(vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrDst(vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrSrc(vcpu::OperandSize opSize, ast::Expression::Ref src);

            bool EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral);
            bool EmitRegisterLiteralWithAddrMode(ast::RegisterLiteral::Ref regLiteral, uint8_t addrMode);
            // Special version which will also output the reg|mode byte
            bool EmitNumericLiteralForInstr(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitStringLiteral(vcpu::OperandSize opSize, ast::StringLiteral::Ref strLiteral);
            bool EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitLabelAddress(ast::Identifier::Ref identifier);
            bool EmitRelativeLabelAddress(ast::Identifier::Ref identifier, vcpu::OperandSize opSize);
            bool EmitDereference(ast::DeReferenceExpression::Ref expression);


            bool EmitByte(uint8_t byte);
            bool EmitWord(uint16_t word);
            bool EmitDWord(uint32_t dword);
            bool EmitLWord(uint64_t lword);
        private:
            CompiledUnit unit;

            // FIXME: replace this...
            //std::vector<uint8_t> outStream;

            struct StructDefinition {
                std::string ident;
                size_t byteSize;
            };

            std::vector<StructDefinition> structDefinitions;
            std::vector<IdentifierAddressPlaceholder> addressPlaceholders;
            std::unordered_map<std::string, IdentifierAddress> identifierAddresses;
            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;

            std::vector<uint8_t> linkdata;
        };
    }
}



#endif //COMPILER_H
