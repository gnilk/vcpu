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
            bool ProcessMoveInstr(ast::MoveInstrStatment::Ref stmt);

            bool EmitOpCodeForSymbol(const std::string &symbol);
            bool EmitInstrDst(OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrSrc(OperandSize opSize, ast::Expression::Ref src);

            bool EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral);
            bool EmitNumericLiteral(OperandSize opSize, ast::NumericLiteral::Ref numLiteral);

            bool EmitByte(uint8_t byte);
            bool EmitWord(uint16_t word);
            bool EmitDWord(uint32_t dword);
            bool EmitLWord(uint64_t lword);
        private:
            // FIXME: replace this...
            std::vector<uint8_t> outStream;
        };
    }
}



#endif //COMPILER_H
