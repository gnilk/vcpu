//
// Created by gnilk on 20.12.23.
//

#ifndef COMPILEDUNIT_H
#define COMPILEDUNIT_H

#include <string>
#include <vector>
#include <stdint.h>

#include "InstructionSet.h"
#include "Segment.h"

namespace gnilk {
    namespace assembler {
        class CompiledUnit {
        public:
            CompiledUnit() = default;
            virtual ~CompiledUnit() = default;

            void Clear();
            bool CreateEmptySegment(const std::string &name);
            bool GetOrAddSegment(const std::string &name, uint64_t address);
            bool SetActiveSegment(const std::string &name);
            bool HaveSegment(const std::string &name);
            Segment::Ref GetActiveSegment();

            bool MergeSegments(const std::string &dst, const std::string &src);

            size_t GetSegments(std::vector<Segment::Ref> &outSegments);

            const Segment::Ref GetSegment(const std::string segName);

            size_t GetSegmentEndAddress(const std::string &name);
            bool WriteByte(uint8_t byte);
            void ReplaceAt(uint64_t offset, uint64_t newValue);
            void ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize);

            void SetBaseAddress(uint64_t address);
            uint64_t GetBaseAddress();
            uint64_t GetCurrentWritePtr();

            const std::vector<uint8_t> &Data();
        protected:
            // Consider exposing the segments!!   it becomes tideous to access them through the unit...
            uint64_t baseAddress = 0x0;
            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;
        };

    }
}

#endif //COMPILEDUNIT_H
