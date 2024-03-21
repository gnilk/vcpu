//
// Created by gnilk on 07.03.24.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include <vector>

#include "CompiledUnit.h"
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
            Identifier &GetExport(const std::string &ident) override;
            const std::unordered_map<std::string, Identifier> &GetExports() override {
                return publicIdentifiers;
            }

            bool HasStructDefinintion(const std::string &typeName) override;
            void AddStructDefinition(const StructDefinition &structDefinition) override;
            const StructDefinition &GetStructDefinitionFromTypeName(const std::string &typeName) override;
            const std::vector<StructDefinition> &StructDefinitions() override;



            CompiledUnit &CreateUnit();
            const std::vector<CompiledUnit> &GetUnits() {
                return units;
            }

            CompiledUnit &CurrentUnit() {
                if (units.empty()) {
                    return CreateUnit();
                }
                return units.back();
            }

            void Merge();

            // Segment handling
            bool EnsureChunk();
            bool CreateEmptySegment(const std::string &name);
            bool GetOrAddSegment(const std::string &name);
            bool SetActiveSegment(const std::string &name);
            bool HaveSegment(const std::string &name);
            Segment::Ref GetActiveSegment();
            size_t GetSegments(std::vector<Segment::Ref> &outSegments);
            const Segment::Ref GetSegment(const std::string segName);
            size_t GetSegmentEndAddress(const std::string &name);

            // TEMP
            void MergeAllSegments(std::vector<uint8_t> &out);

            uint64_t GetCurrentWriteAddress();
            size_t Write(const std::vector<uint8_t> &data);
            std::vector<uint8_t> &Data() {
                return outputdata;
            }

        protected:
            // TEMP:
            std::vector<uint8_t> outputdata;

            // Should be list...
            //CompiledUnit unit;
            std::vector<CompiledUnit> units;

            // Segments and chunks are here as they span multiple units..
            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            // This is a type-definition - should they always be exported?
            std::vector<StructDefinition> structDefinitions;
            // identifiers explicitly marked for export goes here...
            std::unordered_map<std::string, Identifier> publicIdentifiers;

        };

    };
}



#endif //CONTEXT_H
