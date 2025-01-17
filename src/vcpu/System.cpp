//
// Created by gnilk on 08.04.2024.
//

#include <string.h>
#include "System.h"
#include "MemorySubSys/FlashBus.h"
#include "MemorySubSys/RamBus.h"
#include "MemorySubSys/HWMappedBus.h"
#include "VirtualCPU.h"

using namespace gnilk;
using namespace gnilk::vcpu;


//
// This is actually quite hard to follow...
// BUT - the MMU talks SoC - about the memory configuration (defined below)
// The SoC is auto-initialized on access
//

static MemoryRegionConfiguration memoryConfiguration[]={
        // Default RAM region
        {
                .regionFlags = kMemRegion_Default_Ram,
                .vAddrStart = 0x00,
                .sizeBytes = 65535,
        },
        // A region with HW mapped memory
        {
                .regionFlags = kMemRegion_Default_HWMapped,
                .vAddrStart = 0x0100'0000'0000'0000,
                .sizeBytes = 65535,
        },
        // Default Flash region
        {
            .regionFlags = kMemRegion_Default_Flash,
            .vAddrStart = 0x0200'0000'0000'0000,
            .sizeBytes = 65535,
        },
};


SoC &SoC::Instance() {
    static SoC glbInstance;
    if (!glbInstance.isInitialized) {
        glbInstance.Initialize();
    }
    return glbInstance;
}

void SoC::Initialize() {
    SetDefaults();
    for(int i=0;i<VCPU_SOC_MAX_CORES;i++) {
        if (cores[i].cpu != nullptr) continue;
        cores[i].cpu = std::make_shared<VirtualCPU>();
        cores[i].cpu->memoryUnit.Initialize(i);
        cores[i].isValid = true;
    }
    isInitialized = true;
}

void SoC::Reset() {
    for(int i=0; i < VCPU_MEM_MAX_REGIONS; i++) {
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

    CreateMemoryRegionsFromConfig(memoryConfiguration);
/*
    CreateDefaultRAMRegion(0);
    CreateDefaultHWRegion(1);
    CreateDefaultFlashRegion(2);
*/
}

void SoC::CreateMemoryRegionsFromConfig(std::span<MemoryRegionConfiguration> configs) {
    size_t idxRegion = 0;
    for(auto &c : configs) {
        // Setup the region from configuration here
        auto &region = GetRegion(idxRegion);
        region.flags = c.regionFlags;
        region.vAddrStart = c.vAddrStart;
        region.vAddrEnd = c.vAddrStart + c.sizeBytes;

        // FIXME: This should perhaps be done differently...  but yeah - let's refactor when we need it...
        if ((c.regionFlags == kMemRegion_Default_Ram) || (c.regionFlags == kMemRegion_Default_Flash)) {
            region.szPhysical = c.sizeBytes;
            region.ptrPhysical = new uint8_t[c.sizeBytes];
            memset(region.ptrPhysical, 0, region.szPhysical);
        }

        switch(c.regionFlags) {
            case kMemRegion_Default_Ram :
                region.bus = RamBus::Create(new RamMemory(region.ptrPhysical, region.szPhysical));
                break;
            case kMemRegion_Default_Flash :
                region.bus = FlashBus::Create(new RamMemory(region.ptrPhysical, region.szPhysical));
                break;
            case kMemRegion_Default_HWMapped :
                region.bus = HWMappedBus::Create();
                break;
        }
        idxRegion++;
    }
}

// Deprecated - keeping them here for the time beeing...
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
    for(int i=0; i < VCPU_MEM_MAX_REGIONS; i++) {
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
