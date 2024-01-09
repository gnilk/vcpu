//
// Created by gnilk on 09.01.2024.
//
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "MemoryUnit.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
    DLL_EXPORT int test_mmu(ITesting *t);
    DLL_EXPORT int test_mmu_isvalid(ITesting *t);
    DLL_EXPORT int test_mmu_allocate(ITesting *t);
}

static uint8_t ram[512*1024] = {};

DLL_EXPORT int test_mmu(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_mmu_isvalid(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, 512*1024);
    TR_ASSERT(t, !mmu.IsAddressValid(0x00));
    return kTR_Pass;
}

DLL_EXPORT int test_mmu_allocate(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, 512*1024);
    auto addr = mmu.AllocatePage();
    auto pagePtr = mmu.GetPageForAddress(addr);
    TR_ASSERT(t, pagePtr != nullptr);

    auto addr2 = mmu.AllocatePage();
    auto pagePtr2 = mmu.GetPageForAddress(addr2);
    TR_ASSERT(t, pagePtr2 != nullptr);

    return kTR_Pass;
}

