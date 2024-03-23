//
// Created by gnilk on 20.12.23.
//

#ifndef COMPILEDUNIT_H
#define COMPILEDUNIT_H

#include <string>
#include <vector>
#include <stdint.h>

#include "ast/ast.h"
#include "StmtEmitter.h"
#include "InstructionSet.h"
#include "Linker/Segment.h"
#include "Identifiers.h"


namespace gnilk {
    namespace assembler {
        // Forward declare so we can use the reference but avoid circular references
        class Context;

        // This should represent a single compiler unit
        // Segment handling should be moved out from here -> they belong to the context as they are shared between compiled units
        class CompileUnit : public IPublicIdentifiers {
        public:
            using Ref = std::shared_ptr<CompileUnit>;
        public:
            CompileUnit() = default;
            virtual ~CompileUnit() = default;

            void Clear();

            bool ProcessASTStatement(IPublicIdentifiers *iPublicIdentifiers, ast::Statement::Ref statement);
            bool EmitData(IPublicIdentifiers *iPublicIdentifiers);

            size_t Write(const std::vector<uint8_t> &data);

            const std::vector<EmitStatementBase::Ref> &GetEmitStatements() {
                return emitStatements;
            }

            // Segment handling
            bool EnsureChunk();
            bool CreateEmptySegment(const std::string &name);
            bool GetOrAddSegment(const std::string &name, uint64_t address);
            bool SetActiveSegment(const std::string &name);
            bool HaveSegment(const std::string &name);
            Segment::Ref GetActiveSegment();
            size_t GetSegments(std::vector<Segment::Ref> &outSegments);
            const Segment::Ref GetSegment(const std::string segName);
            size_t GetSegmentEndAddress(const std::string &name);
            uint64_t GetCurrentWriteAddress();
        public: // Local identifiers
            bool HasConstant(const std::string &name);
            void AddConstant(const std::string &name, ast::ConstLiteral::Ref constant);
            ast::ConstLiteral::Ref GetConstant(const std::string &name);

            bool HasIdentifier(const std::string &ident);
            void AddIdentifier(const std::string &ident, const Identifier &idAddress);
            Identifier::Ref GetIdentifier(const std::string &ident);
            const std::unordered_map<std::string, Identifier::Ref> &GetIdentifiers() {
                return identifierAddresses;
            }

        public: // IPublicIdentifiers
            // This is just a proxy so we have a nicer interface in the EmitStatement classes...
            bool HasStructDefinintion(const std::string &typeName) override {
                return publicHandler->HasStructDefinintion(typeName);
            }
            void AddStructDefinition(const StructDefinition &structDefinition) override {
                return publicHandler->AddStructDefinition(structDefinition);
            }
            const StructDefinition::Ref GetStructDefinitionFromTypeName(const std::string &typeName) override {
                return publicHandler->GetStructDefinitionFromTypeName(typeName);
            }
            const std::vector<StructDefinition::Ref> &StructDefinitions() override {
                return publicHandler->StructDefinitions();
            }
            bool HasExport(const std::string &ident) override {
                return publicHandler->HasExport(ident);
            }
            void AddExport(const std::string &ident) override {
                publicHandler->AddExport(ident);
                // Add to our own list so we can mark exports globally at the end of the unit handling where they do belong
                exports.push_back(ident);
            }
            void AddForwardDeclExport(const std::string &ident) {
                exports.push_back(ident);
            }
            ExportIdentifier::Ref GetExport(const std::string &ident) override {
                return publicHandler->GetExport(ident);
            }
            const std::unordered_map<std::string, ExportIdentifier::Ref> &GetExports() override {
                return publicHandler->GetExports();
            }

        private:
            IPublicIdentifiers *publicHandler = nullptr;
            // TEMP:
            std::vector<uint8_t> outputdata;


            std::vector<EmitStatementBase::Ref> emitStatements;

            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            std::vector<std::string> exports;   // we export these things...
            std::unordered_map<std::string, Identifier::Ref> identifierAddresses;
            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;
        };
    }
}

#endif //COMPILEDUNIT_H
