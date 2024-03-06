//
// Created by gnilk on 20.12.23.
//

#ifndef SEGMENT_H
#define SEGMENT_H

#include <stdint.h>

#include <memory>
#include <vector>
#include <string>

#include "InstructionSet.h"

namespace gnilk {
    namespace assembler {
        class CompiledUnit;

        // The ELF Writer would call this a 'Section'
        class Segment {
            friend CompiledUnit;
        public:
            // This is what the ELF Writer call's a 'segment'
            class DataChunk {
                friend Segment;
            public:
                using Ref = std::shared_ptr<DataChunk>;

                DataChunk() = default;
                virtual ~DataChunk() = default;

                const std::vector<uint8_t> &Data() const;
                const void *DataPtr() const;
                size_t Size() const;
                uint64_t LoadAddress() const;
                uint64_t EndAddress() const;
                void SetLoadAddress(uint64_t newLoadAddress);

                void WriteByte(uint8_t byte);
                void ReplaceAt(uint64_t offset, uint64_t newValue);
                void ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize);

                uint64_t GetCurrentWritePtr();
            protected:
                uint64_t loadAddress = 0;
                std::vector<uint8_t> data;
            };

            using Ref = std::shared_ptr<Segment>;
        public:
            Segment(const std::string &segName, uint64_t address);
            virtual ~Segment() = default;

            void CopyFrom(const Segment::Ref other);
            bool CreateChunk(uint64_t loadAddress);

            const std::string &Name() const;
            const std::vector<DataChunk::Ref> &DataChunks();
            DataChunk::Ref ChunkFromAddress(uint64_t address);
            DataChunk::Ref CurrentChunk();
            void SetLoadAddress(uint64_t newLoadAddress);
            uint64_t StartAddress();
            uint64_t EndAddress();


            void WriteByte(uint8_t byte);
            void ReplaceAt(uint64_t offset, uint64_t newValue);
            void ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize);


        protected:
            std::string name = {};
            uint64_t loadAddress = 0;
            std::vector<uint8_t> data;

            DataChunk::Ref currentChunk = nullptr;
            std::vector<DataChunk::Ref> chunks;
        };

    }
}

#endif //SEGMENT_H
