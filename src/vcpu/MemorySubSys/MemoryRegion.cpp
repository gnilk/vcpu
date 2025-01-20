//
// Created by gnilk on 20.01.25.
//

#include <stdlib.h>
#include <string.h>

#include "MemoryRegion.h"
#include "RamBus.h"
#include "FlashBus.h"

using namespace gnilk;
using namespace gnilk::vcpu;

void MemoryRegion::Resize(size_t newSize) {

    if (!((flags == kMemRegion_Default_Ram) || (flags == kMemRegion_Default_Flash))) {
        return;
    }
    szPhysical = newSize;

    if (ptrPhysical != nullptr) {
        delete []static_cast<uint8_t *>(ptrPhysical);
    }

    ptrPhysical = new uint8_t[newSize];
    memset(ptrPhysical, 0, szPhysical);

    if (bus != nullptr) {
        bus = nullptr;
    }

    if (flags == kMemRegion_Default_Ram) {
        bus = RamBus::Create(new RamMemory(ptrPhysical, szPhysical));
    } else {
        bus = FlashBus::Create(new RamMemory(ptrPhysical, szPhysical));
    }

    vAddrEnd = vAddrStart + szPhysical;

}