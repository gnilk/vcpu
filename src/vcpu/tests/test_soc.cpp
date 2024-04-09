//
// Created by gnilk on 09.04.24.
//

#include <stdint.h>
#include <vector>
#include <filesystem>
#include <testinterface.h>

#include "System.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
DLL_EXPORT int test_soc(ITesting *t);
DLL_EXPORT int test_soc_defaults(ITesting *t);
DLL_EXPORT int test_soc_getflashregion(ITesting *t);
DLL_EXPORT int test_soc_resetram(ITesting *t);
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


