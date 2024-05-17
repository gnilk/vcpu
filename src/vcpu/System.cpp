//
// Created by gnilk on 08.04.2024.
//

#include <string.h>
#include "System.h"
#include "MemorySubSys/FlashBus.h"
#include "MemorySubSys/RamBus.h"
#include "MemorySubSys/HWMappedBus.h"

using namespace gnilk;
using namespace gnilk::vcpu;

SoC &SoC::Instance() {
    static SoC glbInstance;
    if (!glbInstance.isInitialized) {
        glbInstance.Initialize();
    }
    return glbInstance;
}

void SoC::Initialize() {
    SetDefaults();
    isInitialized = true;
}

void SoC::Reset() {
    for(int i=0;i<VCPU_SOC_MAX_REGIONS;i++) {
        if (!(regions[i].flags & kRegionFlag_Valid)) continue;
        if (regions[i].flags & kRegionFlag_NonVolatile) continue;
        if (regions[i].ptrPhysical == nullptr) continue;

        memset(regions[i].ptrPhysical,0,regions[i].szPhysical);
    }
}

void SoC::SetDefaults() {
    //
    // The default SoC memory configuration has 3 regions;
    // 0 - is the 'RAM' region, this defines an address space for regular cache-able RAM, when SOC is reset this is also reset
    // 1 - is a region for HW Mapping
    // 2 - is a non-volatile region simulating 'Flash'
    //
    // You are free to map more regions - like a secondary emulated external RAM region or additional HW...
    //
    CreateDefaultRAMRegion(0);
    CreateDefaultHWRegion(1);
    CreateDefaultFlashRegion(2);
}

void SoC::CreateDefaultRAMRegion(size_t idxRegion) {
    // Setup region 0 - this is the default RAM region
    auto &region = GetRegion(idxRegion);
    region.flags = kRegionFlag_Valid | kRegionFlag_Cache | kRegionFlag_Execute | kRegionFlag_Read | kRegionFlag_Write;
    region.vAddrStart = 0x0000'0000'0000'0000;
    region.vAddrEnd   = 0x0000'000f'ffff'ffff;

    region.cbAccessHandler = nullptr;

    region.szPhysical = 65536;
    region.ptrPhysical = new uint8_t[region.szPhysical];
    // make sure we do this...
    memset(region.ptrPhysical, 0, region.szPhysical);

    // Create RamMemory and bus-handler and assign to this region
    auto bus = RamBus::Create(new RamMemory(region.ptrPhysical, region.szPhysical));
    region.bus = bus;
}

void SoC::CreateDefaultHWRegion(size_t idxRegion) {
    auto &region = GetRegion(idxRegion);
    region.flags = kRegionFlag_Valid | kRegionFlag_Read | kRegionFlag_Write | kRegionFlag_HWMapping;
    region.vAddrStart = 0x0100'0000'0000'0000;          // (uint64_t(1) << VCPU_SOC_REGION_SHIFT)
    region.vAddrEnd   = 0x0100'0000'0000'ffff;          // (uint64_t(1) << VCPU_SOC_REGION_SHIFT)
    region.bus = HWMappedBus::Create();
    region.cbAccessHandler = nullptr;
}

void SoC::CreateDefaultFlashRegion(size_t idxRegion) {
    auto &region = GetRegion(idxRegion);
    region.flags = kRegionFlag_Valid | kRegionFlag_Read | kRegionFlag_Execute |  kRegionFlag_NonVolatile;

    region.vAddrStart = 0x0200'0000'0000'0000;  // (uint64_t(2) << VCPU_SOC_REGION_SHIFT)
    region.vAddrEnd   = 0x0200'0000'0000'ffff;
    region.szPhysical = 65536;
    region.ptrPhysical = new uint8_t[region.szPhysical];
    // make sure we do this...
    memset(region.ptrPhysical, 0, region.szPhysical);

    // Create RamMemory and bus-handler and assign to this region
    auto bus = FlashBus::Create(new RamMemory(region.ptrPhysical, region.szPhysical));
    region.bus = bus;

    region.cbAccessHandler = nullptr;
}

MemoryRegion &SoC::GetRegion(size_t idxRegion) {
    return regions[idxRegion];
}

MemoryRegion *SoC::GetFirstRegionMatching(uint8_t kRegionFlags) {
    for(int i=0;i<VCPU_SOC_MAX_REGIONS;i++) {
        if ((regions[i].flags & kRegionFlags) == kRegionFlags) {
            return &regions[i];
        }
    }
    return nullptr;
}

void SoC::MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end) {
    regions[region].flags = flags | kRegionFlag_Valid;
    regions[region].vAddrStart = start;
    regions[region].vAddrEnd = end;
}

void SoC::MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end, MemoryAccessHandler handler) {
    regions[region].flags = flags | kRegionFlag_Valid;
    regions[region].vAddrStart = start;
    regions[region].vAddrEnd = end;
    regions[region].cbAccessHandler = handler;
}

const MemoryRegion &SoC::GetMemoryRegionFromAddress(uint64_t address) {
    return RegionFromAddress(address);
}
