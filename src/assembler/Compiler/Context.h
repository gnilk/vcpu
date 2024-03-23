//
// Created by gnilk on 07.03.24.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include <vector>

#include "CompileUnit.h"
#include "Linker/Segment.h"
#include "Identifiers.h"
#include "ast/ast.h"


namespace gnilk {
    namespace assembler {

        class Context : public IPublicIdentifiers {
        public:
            Context() = default;
            virtual ~Context() = default;

            void Clear();


            // IPublicIdentifiers
            bool HasExport(const std::string &ident) override;
            void AddExport(const std::string &ident) override;
            ExportIdentifier::Ref GetExport(const std::string &ident) override;
            const std::unordered_map<std::string, ExportIdentifier::Ref> &GetExports() override {
                return exportIdentifiers;
            }

            bool HasStructDefinintion(const std::string &typeName) override;
            void AddStructDefinition(const StructDefinition &structDefinition) override;
            const StructDefinition::Ref GetStructDefinitionFromTypeName(const std::string &typeName) override;
            const std::vector<StructDefinition::Ref> &StructDefinitions() override;



            CompileUnit &CreateUnit();
            const std::vector<CompileUnit> &GetUnits() {
                return units;
            }

            CompileUnit &CurrentUnit() {
                if (units.empty()) {
                    return CreateUnit();
                }
                return units.back();
            }
            void Merge();
            // linker requires write access here - we should probably refactor so that output is supplied to merge...
            std::vector<uint8_t> &Data() {
                return outputData;
            }
            // Used by unit tests...
            size_t GetSegments(std::vector<Segment::Ref> &outSegments);

            // Merge helpers
        protected:
            void MergeAllSegments(std::vector<uint8_t> &out);
            void ReloacteChunkFromUnit(CompileUnit &unit, Segment::DataChunk::Ref srcChunk);
            size_t Write(const std::vector<uint8_t> &data);

        protected:
            // Segment handling, not exposed - these are only used during the merge process...
            bool EnsureChunk();
            bool CreateEmptySegment(const std::string &name);
            bool GetOrAddSegment(const std::string &name);
            bool SetActiveSegment(const std::string &name);
            bool HaveSegment(const std::string &name);
            Segment::Ref GetActiveSegment();
            const Segment::Ref GetSegment(const std::string segName);
            size_t GetSegmentEndAddress(const std::string &name);

            uint64_t GetCurrentWriteAddress();

        protected:
            std::vector<uint8_t> outputData;
            std::vector<CompileUnit> units;

            // Segments and chunks are here as they span multiple units..
            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            // This is a type-definition - should they always be exported?
            std::vector<StructDefinition::Ref> structDefinitions;
            // identifiers explicitly marked for export goes here...
            std::unordered_map<std::string, ExportIdentifier::Ref> exportIdentifiers;

        };

    };
}



#endif //CONTEXT_H
