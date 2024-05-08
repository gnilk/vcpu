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
        class Context;
        class CompileUnit;

        // The ELF Writer would call this a 'Section'
        class Segment {
            friend Context;
            friend CompileUnit;
        public:
            // This is what the ELF Writer call's a 'segment'
            class DataChunk {
                friend Segment;
            public:
                using Ref = std::shared_ptr<DataChunk>;
            protected:
                static const uint64_t LOAD_ADDR_APPEND = std::numeric_limits<uint64_t>::max();
                //static const uint64_t LOAD_ADDR_APPEND = 0;
            public:
                DataChunk() = default;
                virtual ~DataChunk() = default;

                bool IsEmpty() const;

                const std::vector<uint8_t> &Data() const;
                const void *DataPtr() const;
                size_t Size() const;
                bool Empty() const;
                bool IsLoadAddressAppend() const;
                uint64_t LoadAddress() const;
                uint64_t EndAddress() const;
                void SetLoadAddress(uint64_t newLoadAddress);

                size_t Write(const std::vector<uint8_t> &data);
                size_t WriteByte(uint8_t byte);
                void ReplaceAt(uint64_t offset, uint64_t newValue);
                void ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize);
                uint64_t GetRelativeWriteAddress();
                uint64_t GetCurrentWriteAddress();
            protected:
                uint64_t loadAddress = LOAD_ADDR_APPEND;
                std::vector<uint8_t> data;
            }; // class DataChunk

            enum class kSegmentType {
                Code,
                Data,
            };
            using Ref = std::shared_ptr<Segment>;
        public:
            Segment(kSegmentType type);
            Segment(kSegmentType type, uint64_t address);
            virtual ~Segment() = default;

            void CopyFrom(const Segment::Ref other);
            bool CreateChunk(uint64_t loadAddress);
            void AddChunk(DataChunk::Ref chunk);

            static std::pair<bool, kSegmentType> TypeFromString(const std::string &typeName);
            static const std::string &TypeName(Segment::kSegmentType type);

            kSegmentType Type() const;
            const std::string &Name() const;

            // Chunks are already sorted!
            const std::vector<DataChunk::Ref> &DataChunks();
            DataChunk::Ref ChunkFromAddress(uint64_t address);

            // Will resort after creating
            DataChunk::Ref CurrentChunk();
            void SetLoadAddress(uint64_t newLoadAddress);
            uint64_t StartAddress();
            uint64_t EndAddress();



            size_t Write(const std::vector<uint8_t> &srcData);
            size_t WriteByte(uint8_t byte);
            uint64_t GetCurrentWriteAddress();
            void ReplaceAt(uint64_t offset, uint64_t newValue);
            void ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize);


        protected:
            kSegmentType type;
            uint64_t loadAddress = 0;
            std::vector<uint8_t> data;

            DataChunk::Ref currentChunk = nullptr;
            std::vector<DataChunk::Ref> chunks;
        };

    }
}

#endif //SEGMENT_H
