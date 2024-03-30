//
// Created by gnilk on 30.03.24.
//

#ifndef VCPU_CPUMEMCACHE_H
#define VCPU_CPUMEMCACHE_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <functional>

namespace gnilk {
    namespace vcpu {

#define GNK_CPU_NUM_CORES 4
#define GNK_L1_CACHE_LINE_SIZE 64
#define GNK_L1_CACHE_NUM_LINES 4

#define GNK_ADDR_DESC_FROM_ADDR(__addr__) (uint64_t(__addr__) & ~(GNK_L1_CACHE_LINE_SIZE-1))
#define GNK_LINE_OFS_FROM_ADDR(__addr__) (uint64_t(__addr__) & (GNK_L1_CACHE_LINE_SIZE-1))

        enum kMESIState {
            kMesi_Modified = 0x01,
            kMesi_Exclusive = 0x02,
            kMesi_Shared = 0x04,
            kMesi_Invalid = 0x08,
        };

        static const std::string &MESIStateToString(kMESIState state) {
            static std::unordered_map<kMESIState, std::string> stateNames = {
                    {kMesi_Modified, "Modified"},
                    {kMesi_Exclusive, "Exclusive"},
                    {kMesi_Shared, "Shared"},
                    {kMesi_Invalid, "Invalid"},
            };
            return stateNames[state];
        }

        // this is my 'mmu' kind of
        class RamMemory {
        public:
            RamMemory(size_t szRam);
            virtual ~RamMemory();

            // Well - we need something...
            void *TranslateAddress(void *ptr) {
                uint64_t dummy = (uint64_t)ptr;
                dummy = dummy & 0xffff;
                return (void *)dummy;
                //return &data[dummy];
            }

            void Write(uint64_t addrDescriptor, const void *src);
            void Read(void *dst, uint64_t addrDescriptor);
        private:
            uint8_t *data = nullptr;
        };


        // The bus is used to send messages
        // There is only one bus
        class DataBus {
        public:
            // See: https://en.wikipedia.org/wiki/MESI_protocol
            enum class kMemOp {
                kProcRd,    // Processor Read Request
                kProcWr,    // Processor Write Request
                kBusRd,     // Bus requesting to read memory
                kBusWr,     // Bus requesting to write memory
                kBusRdX,    // Bus requesting to write memory to cache
            };
            using MessageHandler = std::function<kMESIState(kMemOp op, uint8_t sender, uint64_t addrDescriptor)>;

            struct MemBusSnooper {
                uint8_t idCore = 0;
                MessageHandler cbOnMessage = nullptr;
            };
        public:
            DataBus(RamMemory &memory) : ram(memory) {}
            virtual ~DataBus() = default;

            void Subscribe(uint8_t idCore, MessageHandler cbOnMessage);
            kMESIState SendMessage(kMemOp, uint8_t sender, uint64_t addrDescriptor);

            kMESIState BroadCastRead(uint8_t idCore, uint64_t addrDescriptor);
            void BroadCastWrite(uint8_t idCore, uint64_t addrDescriptor);

            void WriteMemory(uint64_t addrDescriptor, const void *src);
            void ReadMemory(void *dst, uint64_t addrDescriptor);



        protected:
            // ???
            RamMemory &ram;
            size_t nextSubscriber = 0;
            std::array<MemBusSnooper, GNK_CPU_NUM_CORES> subscribers = {};
        };

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
            size_t CopyFromLine(void *dst, int idxLine, uint16_t offset, size_t nBytes);
            size_t CopyToLine(int idxLine, uint16_t offset, const void *src, size_t nBytes);

            void ReadLineData(void *dst, int idxLine);
            void WriteLineData(int idxLine, const void *src, uint64_t addrDescriptor, kMESIState state);

            void DumpCacheLines();
        protected:
            std::array<CacheLine, GNK_L1_CACHE_NUM_LINES> lines = {};
        };

        // This is attached to each 'core'
        class CacheController : public MemoryBase {
        public:
            CacheController(DataBus &dataBus, uint8_t coreIdentifier);
            virtual ~CacheController() = default;

            int32_t Read(void *dst, const void *src, size_t nBytesToRead);
            int32_t Write(void *dst, const void *src, size_t nBytesToWrite);

            const Cache& GetCache() {
                return cache;
            }
            void Dump();
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
            DataBus &bus;
            uint8_t idCore;
            Cache cache;
        };
    }
}

#endif //VCPU_CPUMEMCACHE_H
