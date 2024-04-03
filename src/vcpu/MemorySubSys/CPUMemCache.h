//
// Created by gnilk on 30.03.24.
//

#ifndef VCPU_CPUMEMCACHE_H
#define VCPU_CPUMEMCACHE_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <functional>

#include "DataBus.h"

namespace gnilk {
    namespace vcpu {

        class MemoryBase {
        public:
            MemoryBase() = default;
            virtual ~MemoryBase() = default;
            virtual int32_t Read(void *dst, const void *src, size_t nBytesToRead) {
                return -1;
            }
            virtual int32_t Write(void *dst, const void *src, size_t nBytesToWrite) {
                return -1;
            }
        };


        // The cache is exclusively for emulated RAM transfers - NO external memory mappings ends up here!
        class Cache { ;
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
            size_t CopyFromLine(uint64_t dstAddress, int idxLine, uint16_t offset, size_t nBytes);
            size_t CopyToLine(int idxLine, uint16_t offset, const uint64_t srcAddress, size_t nBytes);

            void ReadLineData(void *dst, int idxLine);
            void WriteLineData(int idxLine, const void *src, uint64_t addrDescriptor, kMESIState state);

            void DumpCacheLines() const;
        protected:
            std::array<CacheLine, GNK_L1_CACHE_NUM_LINES> lines = {};
        };

        // This is attached to each 'core'
        // The cache is exclusively for emulated RAM transfers - NO external memory mappings ends up here!
        class CacheController : public MemoryBase {
        public:
            CacheController() = default;
            virtual ~CacheController() = default;

            void Initialize(uint8_t coreIdentifier);

            // So read/write are essentially the same - the only difference is that we track the
            // 'writes' as destructive - and hence, if any other core operates on the same address
            // their caches will be updated..  Read's however are not destructive - as long as they are
            // not written to..  Ergo - there is a semantic difference between read/write operations
            // (even though their interface is the same and on a basic level there is just a memcpy)
            int32_t Read(uint64_t dstAddress, const uint64_t srcAddress, size_t nBytesToRead);
            int32_t Write(uint64_t dstAddress, const uint64_t srcAddress, size_t nBytesToWrite);

            void Flush();

            const Cache& GetCache() {
                return cache;
            }
            int GetInvalidLineCount() const;
            void Dump() const;
        protected:

            kMESIState OnDataBusMessage(DataBus::kMemOp op, uint8_t sender, uint64_t addrDescriptor);
            int32_t ReadLine(uint64_t addrDescriptor, kMESIState state);
        protected:
            kMESIState OnMsgBusRd(uint64_t addrDescriptor);
            void OnMsgBusWr(uint64_t addrDescriptor);
        private:
            void WriteMemory(int idxLine);
            void ReadMemory(int idxLine, uint64_t addrDescriptor, kMESIState state);

        private:
            uint8_t idCore;
            Cache cache;
        };
    }
}

#endif //VCPU_CPUMEMCACHE_H
