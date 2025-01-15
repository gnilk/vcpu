//
// Created by gnilk on 14.01.25.
//
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "MemorySubSys/MemoryUnit.h"
#include "MemorySubSys/PageAllocator.h"
#include "MemorySubSys/MemoryRegion.h"
#include "MemorySubSys/RamBus.h"

using namespace gnilk;
using namespace gnilk::vcpu;

#define MMU_MAX_MEM (4096*4096)

extern "C" {
DLL_EXPORT int test_memregion(ITesting *t);
DLL_EXPORT int test_memregion_simple(ITesting *t);
}

DLL_EXPORT int test_memregion(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_memregion_simple(ITesting *t) {
    MemoryRegion region;

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

    uint8_t buf[256] = {};
    for(int i=0;i<256;i++) {
        buf[i] = i;
    }
    region.bus->WriteData(0, buf, 255);


    return kTR_Pass;
}


