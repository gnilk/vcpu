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
            ExportIdentifier::Ref GetExport(const std::string &ident) const;

            const std::unordered_map<std::string, ExportIdentifier::Ref> &GetExports() override {
                return exportIdentifiers;
            }
            const std::unordered_map<std::string, ExportIdentifier::Ref> &GetExports() const {
                return exportIdentifiers;
            }

            bool HasStructDefinintion(const std::string &typeName) override;
            void AddStructDefinition(const StructDefinition &structDefinition) override;
            const StructDefinition::Ref GetStructDefinitionFromTypeName(const std::string &typeName) override;
            const std::vector<StructDefinition::Ref> &StructDefinitions() override;

            bool HasStartAddress() const {
                return (startAddress != nullptr);
            }

            void SetStartAddress(Identifier::Ref startIdentifier) {
                startAddress = startIdentifier;
            }

            Identifier::Ref GetStartAddress() const {
                return startAddress;
            }


            CompileUnit &CreateUnit();
            const std::vector<CompileUnit> &GetUnits() {
                return units;
            }
            const std::vector<CompileUnit> &GetUnits() const {
                return units;
            }

            CompileUnit &CurrentUnit() {
                if (units.empty()) {
                    return CreateUnit();
                }
                return units.back();
            }
            void Merge();
            void MergeSegmentForUnit(CompileUnit &unit, Segment::kSegmentType segType);
            // linker requires write access here - we should probably refactor so that output is supplied to merge...
            std::vector<uint8_t> &Data() {
                return outputData;
            }

            // Merge helpers
        protected:
            size_t ComputeEndAddress();
            void MergeAllSegments(std::vector<uint8_t> &out);
            void ReloacteChunkFromUnit(CompileUnit &unit, Segment::DataChunk::Ref srcChunk);
            size_t Write(const std::vector<uint8_t> &data);

        public:
            // Segment handling, exposed because the linker handles the merge process
            bool EnsureChunk();
            Segment::Ref GetOrAddSegment(Segment::kSegmentType type);
//            bool SetActiveSegment(Segment::kSegmentType type);
            bool HaveSegment(Segment::kSegmentType type);
            Segment::Ref GetActiveSegment();
            const Segment::Ref GetSegment(Segment::kSegmentType type);
            size_t GetSegmentEndAddress(Segment::kSegmentType type);
            std::vector<Segment::Ref> &GetSegments();
            const std::vector<Segment::Ref> &GetSegments() const;
            // Used by unit tests...
            size_t GetSegments(std::vector<Segment::Ref> &outSegments);
            size_t GetSegmentsOfType(Segment::kSegmentType ofType, std::vector<Segment::Ref> &outSegments) const;

            uint64_t GetCurrentWriteAddress();

        protected:
            std::vector<uint8_t> outputData;
            std::vector<CompileUnit> units;

            // FIXME: Segments can't be unordered map - they come at a specific order and must be processed in same order
            //        Segments come in a specific order and address
            std::vector<Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            // This is a type-definition - should they always be exported?
            std::vector<StructDefinition::Ref> structDefinitions;
            std::vector<Identifier> identifiers;
            // identifiers explicitly marked for export goes here...
            std::unordered_map<std::string, ExportIdentifier::Ref> exportIdentifiers;

            Identifier::Ref startAddress = nullptr;

        };

    };
}



#endif //CONTEXT_H
