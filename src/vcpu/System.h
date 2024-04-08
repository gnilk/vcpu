//
// Created by gnilk on 08.04.2024.
//

#ifndef VCPU_SYSTEM_H
#define VCPU_SYSTEM_H

#include <functional>
#include <stdint.h>

#include "MemorySubSys/DataBus.h"
namespace gnilk {
    namespace vcpu {

        static const size_t VCPU_SOC_MAX_REGIONS = 16;      // we have 16 memory regions...

        enum RegionFlags {
            kRegionFlag_Valid = 0x01,
            kRegionFlag_Read = 0x02,
            kRegionFlag_Write = 0x04,
            kRegionFlag_Execute = 0x08,
            kRegionFlag_Cache = 0x10,   // Should this region be cached or not
        };

        using MemoryAccessHandler = std::function<void(DataBus::kMemOp op, uint64_t address)>;
        // Should each region be tied to a memory bus - or is this optional?
        // Also - a region is global across all MMU instances...
        struct MemoryRegion {
            uint64_t rangeStart, rangeEnd;
            MemoryAccessHandler cbAccessHandler = nullptr;
            uint8_t flags = 0;

            // EMU stuff - we can assign physically allocated stuff here
            void *ptrPhysical = nullptr;
            size_t szPhysical = 0;
        };

        // Move these to the 'SoC' level?
        static const uint64_t VCPU_SOC_REGION_MASK = 0x0f00'0000'0000'0000;
        static const uint64_t VCPU_SOC_REGION_SHIFT = (64-8);

        // This is the full valid address space for 64bit; note: this is a limitation I set due to table space and what not..
        // Better table layout would make this much more 'dynamic'
        static const uint64_t VCPU_SOC_ADDR_MASK = 0x000f'ffff'ffff'ffff;


        // This defines the System-on-a-Chip, it holds all the on-chip hardware..
        class SoC {
        public:
            SoC() = default;
            virtual ~SoC() = default;

            void MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end);
            void MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end, MemoryAccessHandler handler);
            const MemoryRegion &GetMemoryRegionFromAddress(uint64_t address);


            __inline uint8_t constexpr RegionFromAddress(uint64_t address) {
                uint8_t region = (address & VCPU_SOC_REGION_MASK) >> (VCPU_SOC_REGION_SHIFT);
                return region;
            }

        private:
            MemoryRegion regions[VCPU_SOC_MAX_REGIONS];
            // FIXME: Define 'cores' here..

        };

    }
}

#endif //VCPU_SYSTEM_H
