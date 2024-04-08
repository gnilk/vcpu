//
// Created by gnilk on 08.04.2024.
//

#include "System.h"

using namespace gnilk;
using namespace gnilk::vcpu;

void SoC::MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end) {
    regions[region].flags = flags | kRegionFlag_Valid;
    regions[region].rangeStart = start;
    regions[region].rangeEnd = end;
}

void SoC::MapRegion(uint8_t region, uint8_t flags, uint64_t start, uint64_t end, MemoryAccessHandler handler) {
    regions[region].flags = flags | kRegionFlag_Valid;
    regions[region].rangeStart = start;
    regions[region].rangeEnd = end;
    regions[region].cbAccessHandler = handler;
}

const MemoryRegion &SoC::GetMemoryRegionFromAddress(uint64_t address) {
    auto idxRegion = RegionFromAddress(address);
    return regions[idxRegion];
}
