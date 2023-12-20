//
// Created by gnilk on 15.12.23.
//

#ifndef COMPILER_H
#define COMPILER_H

#include "ast/ast.h"
#include "Linker/CompiledUnit.h"

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
            bool ProcessMetaStatement(ast::MetaStatement::Ref stmt);



            bool EmitOpCodeForSymbol(const std::string &symbol);
            bool EmitInstrOperand(vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrDst(vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrSrc(vcpu::OperandSize opSize, ast::Expression::Ref src);

            bool EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral);
            // Special version which will also output the reg|mode byte
            bool EmitNumericLiteralForInstr(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitLabelAddress(ast::Identifier::Ref identifier);
            bool EmitDereference(ast::DeReferenceExpression::Ref expression);


            bool EmitByte(uint8_t byte);
            bool EmitWord(uint16_t word);
            bool EmitDWord(uint32_t dword);
            bool EmitLWord(uint64_t lword);
        private:
            CompiledUnit unit;

            // FIXME: replace this...
            //std::vector<uint8_t> outStream;

            struct IdentifierAddressPlaceholder {
                std::string ident;
                Segment::Ref segment;
                uint64_t address;
            };
            struct IdentifierAddress {
                Segment::Ref segment;
                uint64_t address;
            };
            std::vector<IdentifierAddressPlaceholder> addressPlaceholders;
            std::unordered_map<std::string, IdentifierAddress> identifierAddresses;


            std::vector<uint8_t> linkdata;
        };
    }
}



#endif //COMPILER_H
