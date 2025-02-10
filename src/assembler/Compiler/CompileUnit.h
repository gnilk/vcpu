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
#include "InstructionSetV1/InstructionSetV1Def.h"
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
            bool ProcessASTStatement(ast::Statement::Ref statement);
            bool EmitData(IPublicIdentifiers *iPublicIdentifiers);

            size_t Write(const std::vector<uint8_t> &data);

            const std::vector<EmitStatementBase::Ref> &GetEmitStatements() {
                return emitStatements;
            }

            // Segment handling
            bool EnsureChunk();
            Segment::Ref CreateEmptySegment(Segment::kSegmentType type);
            Segment::Ref GetOrAddSegment(Segment::kSegmentType type, uint64_t address);
            const Segment::Ref GetSegment(Segment::kSegmentType type);
            size_t GetSegmentEndAddress(Segment::kSegmentType type);
            //bool SetActiveSegment(Segment::kSegmentType type);
            bool HaveSegment(Segment::kSegmentType type);
            Segment::Ref GetActiveSegment();

            size_t GetSegments(std::vector<Segment::Ref> &outSegments) const;
            const std::vector<Segment::Ref> &GetSegments() const;

            uint64_t GetCurrentWriteAddress();
            uint64_t GetCurrentRelativeWriteAddress();
        public: // Local identifiers
            bool HasConstant(const std::string &name);
            void AddConstant(const std::string &name, ast::ConstLiteral::Ref constant);
            ast::ConstLiteral::Ref GetConstant(const std::string &name);

            bool HasIdentifier(const std::string &ident);
            void AddIdentifier(const std::string &ident, const Identifier &idAddress);
            Identifier::Ref GetIdentifier(const std::string &ident);
            const std::unordered_map<std::string, Identifier::Ref> &GetIdentifiers() const {
                return identifierAddresses;
            }

            ImportIdentifier::Ref AddImport(const std::string ident) {
                auto import = std::make_shared<ImportIdentifier>();
                import->name = ident;
                imports[ident] = import;
                return import;
            }
            const std::unordered_map<std::string, ImportIdentifier::Ref> &GetImports() const {
                return imports;
            }

            bool IsExportLocal(const std::string &ident) {
                return (std::find(exports.begin(), exports.end(), ident) != exports.end());
            }

            void AddExportToExportList(const std::string &ident) {
                exports.push_back(ident);
            }

            ExportIdentifier::Ref AddImplicitExport(const std::string &ident) {
                publicHandler->AddExport(ident);
                exports.push_back(ident);
                return publicHandler->GetExport(ident);
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

            std::vector<Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            std::vector<std::string> exports;   // we export these things...
            std::unordered_map<std::string, ImportIdentifier::Ref> imports; // we import these things...

            // Is this used???
            std::unordered_map<std::string, Identifier::Ref> identifierAddresses;
            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;
        };
    }
}

#endif //COMPILEDUNIT_H
