//
// Created by gnilk on 15.12.23.
//

#ifndef COMPILER_H
#define COMPILER_H

#include "ast/ast.h"

namespace gnilk {
    namespace assembler {
        class CompiledUnit {
        public:
            CompiledUnit() = default;
            virtual ~CompiledUnit() = default;

            void Clear();
            bool GetOrAddSegment(const std::string &name, uint64_t address);
            bool SetActiveSegment(const std::string &name);
            const std::string &GetActiveSegment();
            bool MergeSegments(const std::string &dst, const std::string &src);
            size_t GetSegmentSize(const std::string &name);
            bool WriteByte(uint8_t byte);
            void ReplaceAt(uint64_t offset, uint64_t newValue);
            void SetBaseAddress(uint64_t address);
            uint64_t GetBaseAddress();
            uint64_t GetCurrentWritePtr();

            const std::vector<uint8_t> &Data();
        protected:
            // Consider exposing the segments!!   it becomes tideous to access them through the unit...
            class Segment {
                friend CompiledUnit;
            public:
                using Ref = std::shared_ptr<Segment>;
            public:
                Segment(const std::string &segName, uint64_t address) : name(segName), loadAddress(address) {}
                virtual ~Segment() = default;

                const std::vector<uint8_t> &Data() const {
                    return data;
                }
                size_t Size() const {
                    return data.size();
                }
                uint64_t LoadAddress() const {
                    return loadAddress;
                }
                const std::string &Name() const {
                    return name;
                }

                void WriteByte(uint8_t byte) {
                    data.push_back(byte);
                }
                void ReplaceAt(uint64_t offset, uint64_t newValue) {
                    if (data.size() < 8) {
                        return;
                    }
                    if (offset > (data.size() - 8)) {
                        return;
                    }

                    data[offset++] = (newValue>>56 & 0xff);
                    data[offset++] = (newValue>>48 & 0xff);
                    data[offset++] = (newValue>>40 & 0xff);
                    data[offset++] = (newValue>>32 & 0xff);
                    data[offset++] = (newValue>>24 & 0xff);
                    data[offset++] = (newValue>>16 & 0xff);
                    data[offset++] = (newValue>>8 & 0xff);
                    data[offset++] = (newValue & 0xff);
                }

                uint64_t GetCurrentWritePtr() {
                    return data.size();
                }
            protected:
                std::string name = {};
                uint64_t loadAddress = 0;
                std::vector<uint8_t> data;
            };
            uint64_t baseAddress = 0x0;
            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;
        };

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
                std::string segment;
                uint64_t address;
            };
            struct IdentifierAddress {
                std::string segment;
                uint64_t address;
            };
            std::vector<IdentifierAddressPlaceholder> addressPlaceholders;
            std::unordered_map<std::string, IdentifierAddress> identifierAddresses;


            std::vector<uint8_t> linkdata;
        };
    }
}



#endif //COMPILER_H
