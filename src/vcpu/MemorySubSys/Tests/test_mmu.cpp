//
// Created by gnilk on 09.01.2024.
//
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "MemorySubSys/MemoryUnit.h"

using namespace gnilk;
using namespace gnilk::vcpu;

#define MMU_MAX_MEM (4096*4096)

extern "C" {
    DLL_EXPORT int test_mmu(ITesting *t);
    DLL_EXPORT int test_mmu_translate(ITesting *t);
    DLL_EXPORT int test_mmu_isvalid(ITesting *t);
    DLL_EXPORT int test_mmu_allocate(ITesting *t);
    DLL_EXPORT int test_mmu_allocate_many(ITesting *t);
    DLL_EXPORT int test_mmu_free(ITesting *t);
}

// Number of pages currently supported with current page-table defines
static uint8_t ram[MMU_MAX_MEM] = {};

static void DumpBitmapTable(MemoryUnit &mmu, size_t szMax) {
    auto ptrBitmap = mmu.GetPhyBitmapTable();
    auto szBitmapTable = mmu.GetPhyBitmapTableSize();
    // Trunc if > 0
    if (szMax > 0) {
        szBitmapTable = szMax;
    }
    printf("MMU U-Test, PhyBitmap Table, dumping: %zu bytes\n", szBitmapTable);
    for(int i=0;i<szBitmapTable;i++) {
        printf("0x%.2x ", ptrBitmap[i]);
        if ((i & 7) == 7) {
            printf("  ");
        }
        if ((i & 15) == 15) {
            printf("\n");
        }
    }
    printf("\n");
}

DLL_EXPORT int test_mmu(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_mmu_translate(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, MMU_MAX_MEM);
    // We need to make sure that RAM doesn't start at address 0...
    auto virtualAddress = mmu.AllocatePage();

    auto translatedAddress = mmu.TranslateAddress(virtualAddress);
    TR_ASSERT(t, translatedAddress == MemoryUnit::VIRTUAL_ADDR_SPACE_START);
    auto translatedAddress2 = mmu.TranslateAddress(virtualAddress+128);
    TR_ASSERT(t, translatedAddress2 == (MemoryUnit::VIRTUAL_ADDR_SPACE_START + 128));

    // Outside the page - this should not exists
    auto translatedAddress3 = mmu.TranslateAddress(virtualAddress+5000);
    TR_ASSERT(t, translatedAddress3 == 0);

    mmu.FreePage(virtualAddress);

    return kTR_Pass;
}


DLL_EXPORT int test_mmu_isvalid(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, MMU_MAX_MEM);
    TR_ASSERT(t, !mmu.IsAddressValid(0x00));
    return kTR_Pass;
}

DLL_EXPORT int test_mmu_allocate(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, MMU_MAX_MEM);
    auto addr = mmu.AllocatePage();
    auto pagePtr = mmu.GetPageForAddress(addr);
    TR_ASSERT(t, pagePtr != nullptr);

    auto addr2 = mmu.AllocatePage();
    auto pagePtr2 = mmu.GetPageForAddress(addr2);
    TR_ASSERT(t, pagePtr2 != nullptr);

    return kTR_Pass;
}


DLL_EXPORT int test_mmu_allocate_many(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, MMU_MAX_MEM);
    for(int i=0;i<8;i++) {
        auto addr = mmu.AllocatePage();
        auto pagePtr = mmu.GetPageForAddress(addr);
        if (pagePtr == nullptr) {
            printf("Failed @ %d, vadr: %x\n", i, (uint32_t)addr);
            return kTR_Fail;
        }
        TR_ASSERT(t, pagePtr != nullptr);
    }
    // The first 8 physical pages should be marked as allocated
    if (mmu.GetPhyBitmapTable()[0] != 0xff) {
        DumpBitmapTable(mmu,16);
    }
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[0] == 0xff);

    // Allocate another 8...
    for(int i=0;i<8;i++) {
        auto addr = mmu.AllocatePage();
        auto pagePtr = mmu.GetPageForAddress(addr);
        if (pagePtr == nullptr) {
            printf("Failed @ %d, vadr: %x\n", i, (uint32_t)addr);
            return kTR_Fail;
        }
        TR_ASSERT(t, pagePtr != nullptr);
    }
    // The second batch (8 next)..
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[1] == 0xff);

    // Allocate another 16
    for(int i=0;i<16;i++) {
        auto addr = mmu.AllocatePage();
        auto pagePtr = mmu.GetPageForAddress(addr);
        if (pagePtr == nullptr) {
            printf("Failed @ %d, vadr: %x\n", i, (uint32_t)addr);
            return kTR_Fail;
        }
        TR_ASSERT(t, pagePtr != nullptr);
    }
    // Dump for inspection..
    DumpBitmapTable(mmu,16);

    return kTR_Pass;
}

DLL_EXPORT int test_mmu_free(ITesting *t) {
    MemoryUnit mmu;
    mmu.Initialize(ram, MMU_MAX_MEM);
    auto addr = mmu.AllocatePage();
    auto pagePtr = mmu.GetPageForAddress(addr);
    TR_ASSERT(t, pagePtr != nullptr);
    TR_ASSERT(t, pagePtr->ptrPhysical != nullptr);
    TR_ASSERT(t, pagePtr->flags == MMU_FLAGS_RWX);
    TR_ASSERT(t, pagePtr->bytes == VCPU_MMU_PAGE_SIZE);

    // Check that the physical bitmap table has marked the page as allocated
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[0] == 0x01);
    mmu.FreePage(addr);
    // verify that free also released the bitmap table
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[0] == 0x00);
    TR_ASSERT(t, pagePtr->ptrPhysical == nullptr);
    TR_ASSERT(t, pagePtr->flags == 0);
    TR_ASSERT(t, pagePtr->bytes == 0);

    // Now allocate two pages and free one - check if the bitmap table updates correspondingly..
    addr = mmu.AllocatePage();
    auto pagePtr2 = mmu.GetPageForAddress(addr);
    TR_ASSERT(t, pagePtr2 != nullptr);
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[0] == 0x01);
    auto addr3 = mmu.AllocatePage();
    auto pagePtr3 = mmu.GetPageForAddress(addr);
    TR_ASSERT(t, pagePtr3 != nullptr);
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[0] == 0x03);
    mmu.FreePage(addr3);
    TR_ASSERT(t, mmu.GetPhyBitmapTable()[0] == 0x01);


    return kTR_Pass;
}
