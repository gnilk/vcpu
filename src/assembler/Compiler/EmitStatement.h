//
// Created by gnilk on 07.03.24.
//

#ifndef EMITSTATEMENT_H
#define EMITSTATEMENT_H

#include <memory>

#include "InstructionSet.h"
#include "ast/ast.h"
#include "Context.h"

namespace gnilk {
    namespace assembler {
        class Compiler;
        // This encapsulats the data generated for a single statement
        // Once a statement has been processed this is written to the output stream..
        class EmitStatement {
            friend Compiler;
        public:
            using Ref = std::shared_ptr<EmitStatement>;
        public:
            EmitStatement(size_t emitId, Context &useContext, ast::Statement::Ref stmt) : id(emitId), context(useContext), statement(stmt) {}
            virtual ~EmitStatement() = default;
            bool Process();

            bool Finalize();

        protected:
            enum kEmitFlags : uint8_t {
                kEmitNone = 0,
                kEmitOpCode = 1,
                kEmitOpSize = 2,
                kEmitRegMode = 4,
                kEmitDst = 8,
                kEmitSrc = 16,
            };

            friend inline constexpr EmitStatement::kEmitFlags operator |= (EmitStatement::kEmitFlags lhs, const EmitStatement::kEmitFlags rhs) {
                using T = std::underlying_type_t<EmitStatement::kEmitFlags>;
                return static_cast<EmitStatement::kEmitFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
            }

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
            ast::Literal::Ref EvaluateMemberExpression(ast::MemberExpression::Ref expression);

        protected:
            bool EmitOpCodeForSymbol(const std::string &symbol);
            void EmitOpSize(uint8_t opSize);
            void EmitRegMode(uint8_t regMode);
            bool EmitInstrOperand(vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrDst(vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitInstrSrc(vcpu::OperandSize opSize, ast::Expression::Ref src);

            bool EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral);
            bool EmitRegisterLiteralWithAddrMode(ast::RegisterLiteral::Ref regLiteral, uint8_t addrMode);
            // Special version which will also output the reg|mode byte
            bool EmitNumericLiteralForInstr(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitStringLiteral(vcpu::OperandSize opSize, ast::StringLiteral::Ref strLiteral);
            bool EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitLabelAddress(ast::Identifier::Ref identifier, vcpu::OperandSize opSize);
            bool EmitRelativeLabelAddress(ast::Identifier::Ref identifier, vcpu::OperandSize opSize);
            bool EmitDereference(ast::DeReferenceExpression::Ref expression);

            bool EmitByte(uint8_t byte);
            bool EmitWord(uint16_t word);
            bool EmitDWord(uint32_t dword);
            bool EmitLWord(uint64_t lword);

            void WriteByte(uint8_t byte);
        protected:

            Context &context;
            size_t id = 0;              // this is incremented for every statement taken from the body, later the whole vector is sorted based on this
            ast::Statement::Ref statement = nullptr;
            std::vector<uint8_t> data;      // Generated data for this statement
            kEmitFlags emitFlags = kEmitFlags::kEmitNone;                      // What data has been generated - this is used to keep track when a sub-statement wants to change

        };



    }
}

#endif //EMITSTATEMENT_H
