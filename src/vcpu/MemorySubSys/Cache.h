//
// Created by gnilk on 10.04.24.
//

#ifndef VCPU_CACHE_H
#define VCPU_CACHE_H

#include <stdint.h>
#include <array>

#include "MesiBusBase.h"

namespace gnilk {
    namespace vcpu {
        // The cache is exclusively for emulated RAM transfers - NO external memory mappings ends up here!
        class CacheController;

        class Cache {
            friend CacheController;
        public:
            struct CacheLine {
                kMESIState state = kMesi_Invalid;  // we need these, perhaps in a separate array
                int time = 0;       // we need something to track 'oldest' block -> this is the one we toss out
                uint64_t addrDescriptor = 0;    // this is the ptr & ~(CACHE_LINE_SIZE-1)
                uint8_t data[GNK_L1_CACHE_LINE_SIZE];
            };
        public:
            Cache() = default;
            virtual ~Cache() = default;

            int GetNumLines() const;
            int GetLineIndex(uint64_t addrDescriptor);
            int NextLineIndex();

            kMESIState GetLineState(int idxLine) const;
            kMESIState SetLineState(int idxLine, kMESIState newState);

            void ResetLine(int idxLine);

            uint64_t GetLineAddrDescriptor(int idxLine);

            void ReadLineData(void *dst, int idxLine);
            void WriteLineData(int idxLine, const void *src, uint64_t addrDescriptor, kMESIState state);

            void DumpCacheLines() const;
        protected:
            int32_t CopyToLineFromExternal(int idxLine, uint16_t offset, const void *src, size_t nBytes);
            int32_t CopyFromLineToExternal(void *dst, int idxLine, uint16_t offset, size_t nBytes);
        protected:
            std::array<CacheLine, GNK_L1_CACHE_NUM_LINES> lines = {};
        };

    }
}

#endif //VCPU_CACHE_H
