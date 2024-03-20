//
// Created by gnilk on 10.03.2024.
//

#ifndef VCPU_STMTEMITTER_H
#define VCPU_STMTEMITTER_H
#include <memory>

#include "InstructionSet.h"
#include "ast/ast.h"

//
// This should replace EmitStatement.cpp/EmitStatement.h
//

namespace gnilk {
    namespace assembler {
        class Context;
        // The type of 'Emitter' Statement is depending on the ast::Statement::Kind - so we will need a 'factory' method..
        // which is called from the HL compiler and then the emitter is processed. Once all emitters have processed they are finalized.
        // Finalization basically merges the data points into a flat list which is processed by the linker..
        // the flat list is like:
        //  <offset><type><data>
        // The relocation would take place on this list or when it's happening..
        //
        enum class kEmitType : uint8_t {
            kUnknown = 0,
            kCode,
            kData,
            kMeta,
            kComment,
            kRelocationPoint,
            kIdentifier,
            kStruct,
        };

//        // Remove this
//        struct EmitData {
//            kEmitType type = kEmitType::kUnknown;
//            size_t address = 0;
//            kEmitFlags emitFlags;
//
//            // Note: can't be a union - perhaps this should not be here...
//            std::string segmentName ;
//            std::string identifierName;
//            std::vector<uint8_t> data;
//
//        };


        class EmitStatementBase {
        public:
            using Ref = std::shared_ptr<EmitStatementBase>;
        public:
            EmitStatementBase() = default;
            virtual ~EmitStatementBase() = default;

            static EmitStatementBase::Ref Create(ast::Statement::Ref statement);

            virtual bool Process(Context &context) { return false; }
            virtual bool Finalize(Context &context) { return false; }

            kEmitType Kind() {
                return emitType;
            }
            size_t EmitId() {
                return emitid;
            }

        protected:
            enum kEmitFlags : uint8_t {
                kEmitNone = 0,
                kEmitOpCode = 1,
                kEmitOpSize = 2,
                kEmitRegMode = 4,
                kEmitDst = 8,
                kEmitSrc = 16,
            };

            // Evaluation stuff - here?
            ast::Literal::Ref EvaluateConstantExpression(Context &context, ast::Expression::Ref expression);
            ast::Literal::Ref EvaluateBinaryExpression(Context &context, ast::BinaryExpression::Ref expression);
            ast::Literal::Ref EvaluateMemberExpression(Context &context, ast::MemberExpression::Ref expression);

            const std::vector<uint8_t> &Data() {
                return data;
            }


            static bool IsCodeStatement(ast::NodeType nodeType);
            static bool IsDataStatement(ast::NodeType nodeType);

            static size_t GetCurrentAddress() {
                return currentAddress;
            }

            bool EmitByte(uint8_t byte);
            bool EmitWord(uint16_t word);
            bool EmitDWord(uint32_t dword);
            bool EmitLWord(uint64_t lword);
            void WriteByte(uint8_t byte);

        protected:
            // current address we are writing to - this is a calculated value
            static size_t currentAddress;


            ast::Statement::Ref statement = nullptr;

            size_t emitid = 0;
            kEmitType emitType = kEmitType::kUnknown;

            size_t address = 0;
            uint8_t emitFlags = 0;  // note: 'must' use underlying type in order to OR/AND/etc..
            std::vector<uint8_t> data;
            //EmitData emitData = {};
        };

        class EmitMetaStatement : public EmitStatementBase {
        public:
            EmitMetaStatement();
            virtual ~EmitMetaStatement() = default;

            bool Process(Context &context) override;
            bool Finalize(Context &context) override;
        protected:
            bool FinalizeSegment(Context &context);
            bool ProcessMetaStatement(Context &context, ast::MetaStatement::Ref stmt);
        private:
            bool isOrigin = false;
            size_t origin = 0;
            std::string segmentName;
        };

        class EmitCommentStatement : public EmitStatementBase {
        public:
            EmitCommentStatement();
            virtual ~EmitCommentStatement() = default;

            bool Process(Context &context) override;
            bool Finalize(Context &context) override;
        protected:
            std::string text;
        };

        class EmitDataStatement : public EmitStatementBase {
        public:
            EmitDataStatement();
            virtual ~EmitDataStatement() = default;

            bool Process(Context &context) override;
            bool Finalize(Context &context) override;
        protected:
            bool ProcessConstLiteral(Context &context, ast::ConstLiteral::Ref stmt);
            bool ProcessArrayLiteral(gnilk::assembler::Context &context, ast::ArrayLiteral::Ref stmt);
            bool ProcessStringLiteral(vcpu::OperandSize opSize, ast::StringLiteral::Ref strLiteral);
            bool ProcessNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool ProcessStructLiteral(Context &context, ast::StructLiteral::Ref stmt);
        private:
            std::vector<EmitStatementBase::Ref> structMemberStatements;
        };

        class EmitIdentifierStatement : public EmitStatementBase {
        public:
            EmitIdentifierStatement();
            virtual ~EmitIdentifierStatement() = default;

            bool Process(Context &context) override;
            bool Finalize(Context &context) override;
        protected:
            bool ProcessIdentifier(Context &context, ast::Identifier::Ref identifier);
        protected:
            std::string symbol;

            //size_t address;
        };
        class EmitStructStatement : public EmitStatementBase {
        public:
            EmitStructStatement();
            virtual ~EmitStructStatement() = default;

            bool Process(Context &context) override;
            bool Finalize(Context &context) override;
        protected:
            bool ProcessStructStatement(Context &context, ast::StructStatement::Ref stmt);


        };

        class EmitCodeStatement : public EmitStatementBase {
        public:
            EmitCodeStatement();
            virtual ~EmitCodeStatement() = default;
            bool Process(Context &context) override;
            bool Finalize(Context &context) override;

        protected:
            bool ProcessNoOpInstrStmt(ast::NoOpInstrStatment::Ref stmt);
            bool ProcessOneOpInstrStmt(Context &context, ast::OneOpInstrStatment::Ref stmt);
            bool ProcessTwoOpInstrStmt(Context &context, ast::TwoOpInstrStatment::Ref stmt);

            bool EmitOpCodeForSymbol(const std::string &symbol);
            void EmitOpSize(uint8_t opSize);
            void EmitRegMode(uint8_t regMode);
            bool EmitInstrOperand(Context &context, vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref dst);
            bool EmitNumericLiteralForInstr(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral);
            bool EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral);
            bool EmitRegisterLiteralWithAddrMode(ast::RegisterLiteral::Ref regLiteral, uint8_t addrMode);
            bool EmitDereference(Context &context, ast::DeReferenceExpression::Ref expression);
            bool EmitLabelAddress(Context &context, ast::Identifier::Ref identifier, vcpu::OperandSize opSize);
            bool EmitRelativeLabelAddress(Context &context, ast::Identifier::Ref identifier, vcpu::OperandSize opSize);


        private:
            // placeholder stuff here...
            bool haveIdentifier = false;

            // Can store this directly here!
            bool isRelative = false;
            std::string symbol;
            size_t address = 0;         // needed???
            size_t placeholderAddress = 0;
            vcpu::OperandSize opSize;
            //int ofsRelative = 0;

        };
    }
}
#endif //VCPU_STMTEMITTER_H
