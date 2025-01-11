//
// Created by gnilk on 11.01.2025.
//

#ifndef VCPU_MEMORYREGION_H
#define VCPU_MEMORYREGION_H

#include <stdlib.h>
#include <functional>
#include <stdint.h>

#include "BusBase.h"

namespace gnilk {
    namespace vcpu {
        static const size_t VCPU_SOC_MAX_CORES = 1;

        static const size_t VCPU_MEM_MAX_REGIONS = 16;      // we have 16 memory regions...
        // Move these to the 'SoC' level?
        static const uint64_t VCPU_MEM_REGION_MASK = 0x0f00'0000'0000'0000;
        static const uint64_t VCPU_MEM_REGION_SHIFT = (64 - 8);

        // This is the full valid address space for 64bit; note: this is a limitation I set due to table space and what not..
        // Better table layout would make this much more 'dynamic'
        static const uint64_t VCPU_MEM_ADDR_MASK = 0x000f'ffff'ffff'ffff;


        enum RegionFlags {
            kRegionFlag_Valid = 0x01,
            kRegionFlag_Read = 0x02,
            kRegionFlag_Write = 0x04,
            kRegionFlag_Execute = 0x08,
            kRegionFlag_Cache = 0x10,   // Should this region be cached or not
            kRegionFlag_NonVolatile = 0x20, // Non-volatile memory - will be preserved when 'Reset' is called
            kRegionFlag_HWMapping = 0x40,   // This a region which is mapped to hardware
            kRegionFlag_User = 0x80,    // Is this a user or supervisor region
        };

        using MemoryAccessHandler = std::function<void(BusBase::kMemOp op, uint64_t address)>;
        // Should each region be tied to a memory bus - or is this optional?
        // Also - a region is global across all MMU instances...
        struct MemoryRegion {
            uint64_t vAddrStart, vAddrEnd;      // Virtual addresses
            //uint64_t firstVirtualAddr = 0;
            MemoryAccessHandler cbAccessHandler = nullptr;
            uint8_t flags = 0;

            // Cache handling...
            //MesiBusBase::Ref bus = nullptr;
            BusBase::Ref bus = nullptr;

            // EMU stuff - we can assign physically allocated stuff here
            void *ptrPhysical = nullptr;
            size_t szPhysical = 0;
        };

    }
}

#endif //VCPU_MEMORYREGION_H
