//
// Created by gnilk on 11.01.2024.
//

#ifndef DUMMYLINKER_H
#define DUMMYLINKER_H

#include <unordered_map>
#include <vector>
#include "BaseLinker.h"
#include "Compiler/CompileUnit.h"
#include "Compiler/Identifiers.h"

namespace gnilk {
    namespace assembler {
        class DummyLinker : public BaseLinker {
        public:
            DummyLinker() = default;
            virtual ~DummyLinker() = default;

            bool Link(Context &context) override;
            const std::vector<uint8_t> &Data() override;
        protected:
            // Move this to base class?
            bool Merge(const Context &context);
            bool MergeUnits(const Context &sourceContext);
            bool MergeSegmentsInUnit(const Context &sourceContext, const CompileUnit &unit);
            bool MergeChunksInSegment(const Context &sourceContext, const CompileUnit &unit, const Segment::Ref &segment);
            bool RelocateExports(const Context &sourceContext);
            bool RelocateChunkFromUnit(const Context &sourceContext, const CompileUnit &srcUnit, const Segment::DataChunk::Ref &srcChunk);
            bool RelocateIdentifiersInChunk(const CompileUnit &srcUnit, const Segment::DataChunk::Ref &srcChunk);
            bool RelocateExportsInChunk(const Context &sourceContext, const CompileUnit &srcUnit, const Segment::DataChunk::Ref &srcChunk);

            size_t Write(const std::vector<uint8_t> &data);
            bool EnsureChunk();

            // New stuff - will replace a few things
            size_t ComputeFinalSize(const Context &sourceContext);

            Context destContext;
            std::vector<uint8_t> linkedData;
        };
    }
}



#endif //DUMMYLINKER_H
