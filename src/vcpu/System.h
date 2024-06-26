//
// Created by gnilk on 08.04.2024.
//

#ifndef VCPU_SYSTEM_H
#define VCPU_SYSTEM_H

#include <functional>
#include <stdint.h>
#include <vector>

#include "MemorySubSys/CacheController.h"
#include "MemorySubSys/RamBus.h"

namespace gnilk {
    namespace vcpu {

        static const size_t VCPU_SOC_MAX_CORES = 1;
        static const size_t VCPU_SOC_MAX_REGIONS = 16;      // we have 16 memory regions...

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

        // Move these to the 'SoC' level?
        static const uint64_t VCPU_SOC_REGION_MASK = 0x0f00'0000'0000'0000;
        static const uint64_t VCPU_SOC_REGION_SHIFT = (64-8);

        // This is the full valid address space for 64bit; note: this is a limitation I set due to table space and what not..
        // Better table layout would make this much more 'dynamic'
        static const uint64_t VCPU_SOC_ADDR_MASK = 0x000f'ffff'ffff'ffff;

        struct Core {
            bool isValid = false;
            //CPUBase::Ref cpu = nullptr;
            //MMU mmu;
            CacheController cacheController;
        };


        // This defines the System-on-a-Chip, it holds all the on-chip hardware..
        class SoC {
            SoC() = default;
        public:
            virtual ~SoC() = default;

            static SoC &Instance();

            // Reset the System - all non-volatile memory is lost (i.e. non-flash)
            void Reset();

            void MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end);
            void MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end, MemoryAccessHandler handler);

            bool IsAddressCacheable(uint64_t address) {
                auto &region = RegionFromAddress(address);
                if (!(region.flags & kRegionFlag_Cache)) {
                    return false;
                }
                return true;
            }

            MemoryRegion *GetFirstRegionMatching(uint8_t kRegionFlags);
            const MemoryRegion &GetMemoryRegionFromAddress(uint64_t address);

            __inline MemoryRegion &RegionFromAddress(uint64_t address) {
                uint8_t region = (address & VCPU_SOC_REGION_MASK) >> (VCPU_SOC_REGION_SHIFT);
                return regions[region];
            }

            BusBase::Ref GetDataBusForAddress(uint64_t address) {
                if (!HaveRegionForAddress(address)) {
                    return nullptr;
                }
                auto &region = RegionFromAddress(address);
                return region.bus;
            }

            size_t GetCacheableRegions(std::vector<MemoryRegion *> &outRegions) {
                for(int i=0;i<VCPU_SOC_MAX_REGIONS;i++) {
                    if ((regions[i].flags & kRegionFlag_Valid) && (regions[i].flags & kRegionFlag_Cache)) {
                        outRegions.push_back(&regions[i]);
                    }
                }
                return outRegions.size();
            }

            __inline uint8_t constexpr RegionIndexFromAddress(uint64_t address) {
                uint8_t region = (address & VCPU_SOC_REGION_MASK) >> (VCPU_SOC_REGION_SHIFT);
                return region;
            }
            __inline bool constexpr HaveRegionForAddress(uint64_t address) {
                auto idxRegion= RegionIndexFromAddress(address);
                if (idxRegion >= VCPU_SOC_MAX_REGIONS) {
                    return false;
                }
                if (!(regions[idxRegion].flags & kRegionFlag_Valid)) {
                    return false;
                }

                // Outside region address range?
                if (address < regions[idxRegion].vAddrStart) return false;
                if (address > regions[idxRegion].vAddrEnd) return false;

                return true;
            }
        protected:
            void Initialize();
            void SetDefaults();

            MemoryRegion &GetRegion(size_t idxRegion);
            void CreateDefaultRAMRegion(size_t idxRegion);
            void CreateDefaultHWRegion(size_t idxRegion);
            void CreateDefaultFlashRegion(size_t idxRegion);
        private:
            bool isInitialized = false;
            Core cores[VCPU_SOC_MAX_CORES];
            MemoryRegion regions[VCPU_SOC_MAX_REGIONS];
        };

    }
}

#endif //VCPU_SYSTEM_H
