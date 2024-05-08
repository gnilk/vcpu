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

            bool Merge();
            std::vector<uint8_t> &Data() {
                return outputData;
            }

            bool HasStartAddress() const {
                return (startAddressIdentifier != nullptr);
            }

            void SetStartAddress(Identifier::Ref startIdentifier) {
                startAddressIdentifier = startIdentifier;
            }

            Identifier::Ref GetStartAddress() const {
                return startAddressIdentifier;
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

            // IPublicIdentifiers
        public:
            bool HasStructDefinintion(const std::string &typeName) override;
            void AddStructDefinition(const StructDefinition &structDefinition) override;
            const StructDefinition::Ref GetStructDefinitionFromTypeName(const std::string &typeName) override;
            const std::vector<StructDefinition::Ref> &StructDefinitions() override;

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

            // Merge helpers
        protected:
            bool MergeUnits(std::vector<uint8_t> &out);
            void MergeSegmentForUnit(CompileUnit &unit, Segment::kSegmentType segType);
            void FinalMergeOfSegments(std::vector<uint8_t> &out);
            uint64_t ComputeEndAddress();
            bool RelocateChunkFromUnit(CompileUnit &unit, Segment::DataChunk::Ref srcChunk);
            bool RelocateExports();
            size_t Write(const std::vector<uint8_t> &data);

            // Needed by the elf-writer after the merge has been complete...
        public:
            // Segment handling, exposed because the linker handles the merge process
            bool EnsureChunk();

            // Returns
            //   An existing or new segment of the specified type, the returned segment also becomes the active segment
            Segment::Ref GetOrAddSegment(Segment::kSegmentType type);

            // Returns
            //  The active segment
            Segment::Ref GetActiveSegment();

            // Returns
            //  An existing segment of the specific type or null if non exists
            const Segment::Ref GetSegment(Segment::kSegmentType type);

            // Returns
            //  Reference to the list of segments
            std::vector<Segment::Ref> &GetSegments();

            // Returns
            //  Copy of all segment references in 'outSegments' and the size
            size_t GetSegments(std::vector<Segment::Ref> &outSegments);

            // Returns
            //  Copy of all segment references of a specific type in 'outSegments' and the size
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

            Identifier::Ref startAddressIdentifier = nullptr;

        };

    };
}



#endif //CONTEXT_H
