//
// Created by gnilk on 15.12.23.
//

#ifndef COMPILER_H
#define COMPILER_H

#include "ast/ast.h"

namespace gnilk {
    namespace assembler {
        class Compiler {
        public:
            Compiler() = default;
            virtual ~Compiler() = default;

            bool GenerateCode(gnilk::ast::Program::Ref program);
            const std::vector<uint8_t> &Data() {
                return outStream;
            }
        protected:
            bool ProcessStmt(ast::Statement::Ref stmt);
            bool ProcessIdentifier(ast::Identifier::Ref identifier);
            bool ProcessNoOpInstrStmt(ast::NoOpInstrStatment::Ref stmt);
            bool ProcessOneOpInstrStmt(ast::OneOpInstrStatment::Ref stmt);
            bool ProcessTwoOpInstrStmt(ast::TwoOpInstrStatment::Ref stmt);

            bool ReplaceIdentPlaceholdersWithAddresses();

            bool EmitOpCodeForSymbol(const std::string &symbol);
            bool EmitInstrOperand(vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrDst(vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrSrc(vcpu::OperandSize opSize, ast::Expression::Ref src);

            bool EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral);
            bool EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitLabelAddress(ast::Identifier::Ref identifier);


            bool EmitByte(uint8_t byte);
            bool EmitWord(uint16_t word);
            bool EmitDWord(uint32_t dword);
            bool EmitLWord(uint64_t lword);
        private:
            // FIXME: replace this...
            std::vector<uint8_t> outStream;

            struct IdentifierAddressPlaceholder {
                std::string ident;
                uint64_t address;
            };
            std::vector<IdentifierAddressPlaceholder> addressPlaceholders;
            std::unordered_map<std::string, uint64_t> identifierAddresses;
        };
    }
}



#endif //COMPILER_H
