//
// Created by gnilk on 09.04.24.
//

#include <stdint.h>
#include <vector>
#include <filesystem>
#include <string.h>
#include <testinterface.h>
#include "MemorySubSys/MemoryUnit.h"
#include "MemorySubSys/HWMappedBus.h"
#include "System.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
DLL_EXPORT int test_soc(ITesting *t);
DLL_EXPORT int test_soc_defaults(ITesting *t);
DLL_EXPORT int test_soc_getflashregion(ITesting *t);
DLL_EXPORT int test_soc_resetram(ITesting *t);
DLL_EXPORT int test_soc_regionfromaddr(ITesting *t);
DLL_EXPORT int test_soc_hwmapping(ITesting *t);
}

DLL_EXPORT int test_soc(ITesting *t) {
    t->SetPreCaseCallback([](ITesting *) {
        SoC::Instance().Reset();
    });

    return kTR_Pass;
}
DLL_EXPORT int test_soc_defaults(ITesting *t) {

    SoC::Instance().Reset();

    return kTR_Pass;
}
DLL_EXPORT int test_soc_getflashregion(ITesting *t) {
    auto flashMem = SoC::Instance().GetFirstRegionMatching(kRegionFlag_NonVolatile);
    TR_ASSERT(t, flashMem != nullptr);
    TR_ASSERT(t, flashMem->ptrPhysical != nullptr);
    TR_ASSERT(t, flashMem->szPhysical > 0);
    uint8_t *ptrFlash = (uint8_t *)flashMem->ptrPhysical;
    for(int i=0;i<flashMem->szPhysical;i++) {
        ptrFlash[i] = i & 255;
    }
    // Reset the SoC
    SoC::Instance().Reset();

    // Point can still be moved (we don't assure this)
    flashMem = SoC::Instance().GetFirstRegionMatching(kRegionFlag_NonVolatile);
    TR_ASSERT(t, flashMem != nullptr);
    TR_ASSERT(t, flashMem->ptrPhysical != nullptr);
    TR_ASSERT(t, flashMem->szPhysical > 0);
    ptrFlash = (uint8_t *)flashMem->ptrPhysical;

    // Ensure our flash region is intact.
    for(int i=0;i<flashMem->szPhysical;i++) {
        TR_ASSERT(t, ptrFlash[i] == (i & 255));
    }


    return kTR_Pass;
}

DLL_EXPORT int test_soc_resetram(ITesting *t) {
    // only RAM has 'execute'
    auto region = SoC::Instance().GetFirstRegionMatching(kRegionFlag_Read | kRegionFlag_Write | kRegionFlag_Execute);

    TR_ASSERT(t, region != nullptr);
    TR_ASSERT(t, region->ptrPhysical != nullptr);
    TR_ASSERT(t, region->szPhysical > 0);
    uint8_t *ptrMem = (uint8_t *)region->ptrPhysical;
    for(int i=0;i<region->szPhysical;i++) {
        ptrMem[i] = i & 255;
    }
    // Reset the SoC
    SoC::Instance().Reset();

    // Point can still be moved (we don't assure this)
    region = SoC::Instance().GetFirstRegionMatching(kRegionFlag_Read | kRegionFlag_Write | kRegionFlag_Execute);
    TR_ASSERT(t, region != nullptr);
    TR_ASSERT(t, region->ptrPhysical != nullptr);
    TR_ASSERT(t, region->szPhysical > 0);
    ptrMem = (uint8_t *)region->ptrPhysical;

    // Ensure our flash region is intact.
    for(int i=0;i<region->szPhysical;i++) {
        TR_ASSERT(t, ptrMem[i] == 0);
    }
    return kTR_Pass;
}

DLL_EXPORT int test_soc_regionfromaddr(ITesting *t) {
    uint64_t addr = 0x0200'0000'0000'0000;
    auto index = SoC::Instance().RegionIndexFromAddress(addr);
    TR_ASSERT(t, index == 2);
    auto &region = SoC::Instance().GetMemoryRegionFromAddress(addr);

    return kTR_Pass;
}
DLL_EXPORT int test_soc_hwmapping(ITesting *t) {

    static uint8_t hwregs[1024] = {};
    // Note: SOC will reset before this test is executed
    auto region = SoC::Instance().GetFirstRegionMatching(kRegionFlag_HWMapping);
    auto hwbus = std::reinterpret_pointer_cast<HWMappedBus>(region->bus);


    auto onRead = [](void *dst, uint64_t address, size_t nBytes) {
        printf("onRead, %d bytes from addr 0x%x\n", (int)nBytes, (uint32_t)address);
        memcpy(dst, &hwregs[address], nBytes);
    };
    auto onWrite = [](uint64_t address, const void *src, size_t nBytes) {
        printf("onWrite, %d bytes to addr 0x%x\n", (int)nBytes, (uint32_t)address);
        memcpy(&hwregs[address], src, nBytes);
    };

    hwbus->SetReadWriteHandlers(onRead, onWrite);

    MMU mmu;
    mmu.Initialize(0);
    uint64_t addr = 0x0100'0000'0000'0123;
    TR_ASSERT(t, hwregs[0] == 0x00);
    TR_ASSERT(t, hwregs[0x123] == 0x00);

    mmu.Write<uint8_t>(addr, 0x42);
    auto value = mmu.Read<uint8_t>(addr);

    TR_ASSERT(t, value == 0x42);

    return kTR_Pass;

}
