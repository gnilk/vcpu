//
// Created by gnilk on 10.04.24.
//

#ifndef VCPU_BUSBASE_H
#define VCPU_BUSBASE_H

#include <memory>
#include <functional>
#include <string>
#include <stdint.h>

namespace gnilk {
    namespace vcpu {
// Try make this a runtime config -> need to replace array with vector...
#ifndef GNK_CPU_NUM_CORES
#define GNK_CPU_NUM_CORES 4
#endif

#ifndef GNK_L1_CACHE_LINE_SIZE
#define GNK_L1_CACHE_LINE_SIZE 64
#endif
#ifndef GNK_L1_CACHE_NUM_LINES
#define GNK_L1_CACHE_NUM_LINES 4
#endif

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

        // BusBase - base class for all bus classes - overload what is important
        // in case of actual manipulation you either must overwrite Write/Read-Line or Write/Read-Data
        // Line function operate on a full cache line while -Data operate on exact data locations
        // NOTE:
        // -Line functions are used for anything cacheable (as it is called by the cache-controller)
        // -Data functions are used for non-cacheable data
        // Thus, when configuring regions, if a region is non-cacheable (the 'NonVolatile' flag is set) the bus should
        // be configured as 'FlashBus' (which currently implement the -Data functions.
        // If the Bus is cacheable (i.e. 'NonVolatile' is not set) - you should use the 'RamBus'.
        class BusBase {
        public:
            using Ref = std::shared_ptr<BusBase>;
            // See: https://en.wikipedia.org/wiki/MESI_protocol
            enum class kMemOp {
                kProcRd,    // Processor Read Request
                kProcWr,    // Processor Write Request
                kBusRd,     // Bus requesting to read memory
                kBusWr,     // Bus requesting to write memory
                kBusRdX,    // Bus requesting to write memory to cache
            };

            using MessageHandler = std::function<kMESIState(kMemOp op, uint8_t sender, uint64_t addrDescriptor)>;
        public:
            BusBase() = default;
            virtual ~BusBase() = default;

            virtual void Subscribe(uint8_t idCore, MessageHandler cbOnMessage) {}

            virtual kMESIState BroadCastRead(uint8_t idCore, uint64_t addrDescriptor) {
                return kMesi_Invalid;
            }
            virtual void BroadCastWrite(uint8_t idCore, uint64_t addrDescriptor) {}


            virtual void ReadData(void *dst, uint64_t addrDescriptor, size_t nBytes) {}
            virtual void WriteData(uint64_t addrDescriptor, const void *src, size_t nBytes) {}

            virtual void WriteLine(uint64_t addrDescriptor, const void *src) {};
            virtual void ReadLine(void *dst, uint64_t addrDescriptor) {};
        };
    }
}

#endif //VCPU_BUSBASE_H
