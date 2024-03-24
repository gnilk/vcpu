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

            bool Link(const Context &context) override;
            const std::vector<uint8_t> &Data() override;
        protected:
            bool Merge(const Context &context);
            bool MergeAllSegments(const Context &sourceContext);
            bool ReloacteChunkFromUnit(const Context &sourceContext, const CompileUnit &unit, Segment::DataChunk::Ref srcChunk);
            bool EnsureChunk();
            size_t Write(const std::vector<uint8_t> &data);

        private:
            Context destContext;
            std::vector<uint8_t> linkedData;
        };
    }
}



#endif //DUMMYLINKER_H
