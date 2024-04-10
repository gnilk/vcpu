//
// Created by gnilk on 30.03.24.
//

#ifndef VCPU_CPUMEMCACHE_H
#define VCPU_CPUMEMCACHE_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <functional>

#include "RamBus.h"

namespace gnilk {
    namespace vcpu {

        class CacheController;

        // The cache is exclusively for emulated RAM transfers - NO external memory mappings ends up here!
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

        // This is attached to each 'core' through the MMU
        // TO-DO:
        //   - Check the regional-flag if a memory address is cacheable
        class MMU;
        class CacheController {
            friend MMU;
        public:
            CacheController() = default;
            virtual ~CacheController() = default;

            void Initialize(uint8_t coreIdentifier);

            void Touch(const uint64_t address);

            template<typename T>
            int32_t Write(uint64_t address, const T &value) {
                static_assert(std::is_integral_v<T> == true);
                return WriteInternalFromExternal(address, &value, sizeof(T));
            }

            template<typename T>
            T Read(uint64_t address) {
                static_assert(std::is_integral_v<T> == true);
                T value;
                ReadInternalToExternal(&value, address, sizeof(T));
                return value;
            }

            void Flush();

            const Cache& GetCache() {
                return cache;
            }
            int GetInvalidLineCount() const;
            void Dump() const;
        protected:

            kMESIState OnDataBusMessage(MesiBusBase::kMemOp op, uint8_t sender, uint64_t addrDescriptor);
            int32_t ReadLine(MesiBusBase::Ref bus, uint64_t addrDescriptor, kMESIState state);
            kMESIState OnMsgBusRd(uint64_t addrDescriptor);
            void OnMsgBusWr(uint64_t addrDescriptor);

        private:
            int32_t WriteInternalFromExternal(uint64_t address, const void *src, size_t nBytes);
            void ReadInternalToExternal(void *dst, const uint64_t address, size_t nBytes);

            void WriteMemory(MesiBusBase::Ref bus, int idxLine);
            void ReadMemory(MesiBusBase::Ref bus, int idxLine, uint64_t addrDescriptor, kMESIState state);

        private:
            uint8_t idCore = 0;
            Cache cache;
        };
    }
}

#endif //VCPU_CPUMEMCACHE_H
