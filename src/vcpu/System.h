//
// Created by gnilk on 08.04.2024.
//

#ifndef VCPU_SYSTEM_H
#define VCPU_SYSTEM_H

#include <functional>
#include <stdint.h>
#include <vector>

#include "CPUBase.h"
#include "MemorySubSys/RamBus.h"
#include "MemorySubSys/MemoryUnit.h"

namespace gnilk {
    namespace vcpu {

        // FIXME: Move to 'typehelpers.h' or something
        template<typename T>
        inline bool is_instanceof(const auto *ptr) {
            return dynamic_cast<const T*>(ptr) != nullptr;
        }




        // Not sure how to structure this right now
        struct Core {
            bool isValid = false;
            CPUBase::Ref cpu = nullptr;
            MMU mmu;
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
                uint8_t region = (address & VCPU_MEM_REGION_MASK) >> (VCPU_MEM_REGION_SHIFT);
                return regions[region];
            }


            // problem - i need to return something if there is no region...
            template<typename TBus>
            MemoryRegion *GetFirstRegionFromBusType() {
                for(int i=0;i<VCPU_MEM_MAX_REGIONS;i++) {
                    if (!(regions[i].flags & kRegionFlag_Valid)) continue;
                    if (!is_instanceof<TBus>(regions[0].bus.get())) continue;
                    return &regions[i];
                }
                return nullptr;
            }


            BusBase::Ref GetDataBusForAddress(uint64_t address) {
                if (!HaveRegionForAddress(address)) {
                    return nullptr;
                }
                auto &region = RegionFromAddress(address);
                return region.bus;
            }

            size_t GetCacheableRegions(std::vector<MemoryRegion *> &outRegions) {
                for(int i=0; i < VCPU_MEM_MAX_REGIONS; i++) {
                    if ((regions[i].flags & kRegionFlag_Valid) && (regions[i].flags & kRegionFlag_Cache)) {
                        outRegions.push_back(&regions[i]);
                    }
                }
                return outRegions.size();
            }

            __inline uint8_t constexpr RegionIndexFromAddress(uint64_t address) {
                uint8_t region = (address & VCPU_MEM_REGION_MASK) >> (VCPU_MEM_REGION_SHIFT);
                return region;
            }
            __inline bool constexpr HaveRegionForAddress(uint64_t address) {
                auto idxRegion= RegionIndexFromAddress(address);
                if (idxRegion >= VCPU_MEM_MAX_REGIONS) {
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
            MemoryRegion regions[VCPU_MEM_MAX_REGIONS];
        };

    }
}

#endif //VCPU_SYSTEM_H
