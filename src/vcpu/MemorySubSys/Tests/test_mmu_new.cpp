//
// Created by gnilk on 03.04.2024.
//
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "MemorySubSys/MemoryUnit.h"
#include "MemorySubSys/PageAllocator.h"
#include "MemorySubSys/DataBus.h"

using namespace gnilk;
using namespace gnilk::vcpu;

#define MMU_MAX_MEM (4096*4096)

extern "C" {
DLL_EXPORT int test_mmu2(ITesting *t);
DLL_EXPORT int test_mmu2_mapregion(ITesting *t);
DLL_EXPORT int test_mmu2_regionfromaddr(ITesting *t);
DLL_EXPORT int test_mmu2_offsetfromaddr(ITesting *t);
DLL_EXPORT int test_mmu2_ptefromaddr(ITesting *t);
DLL_EXPORT int test_mmu2_copyext(ITesting *t);
DLL_EXPORT int test_mmu2_writeext_read(ITesting *t);
DLL_EXPORT int test_mmu2_write_read(ITesting *t);
DLL_EXPORT int test_mmu2_pagetable_init(ITesting *t);
}


DLL_EXPORT int test_mmu2(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_mapregion_handler(ITesting *t) {
    MMU mmu;

    auto handler = [](DataBus::kMemOp op, uint64_t address) {
        printf("Accessing: 0x%x\n", (uint32_t)address);
    };

    mmu.Initialize();
    mmu.MapRegion(0x01, kRegionFlag_Read, 0x0000'0000'0000'0000, 0x0000'0000'0000'ffff, handler);
    TR_ASSERT(t, mmu.IsAddressValid(0x0100'0000'0000'0000));
    TR_ASSERT(t, mmu.IsAddressValid(0x0100'0000'0000'1234));
    TR_ASSERT(t, mmu.IsAddressValid(0x0100'0000'0000'ffff));
    // These are above the defined address range...
    TR_ASSERT(t, !mmu.IsAddressValid(0x0100'0000'0001'0000));
    TR_ASSERT(t, !mmu.IsAddressValid(0x0100'0000'1234'0000));

    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_mapregion(ITesting *t) {
    MMU mmu;
    mmu.Initialize();
    mmu.MapRegion(0x01, kRegionFlag_Read, 0x0000'0000'0000'0000, 0x0000'0000'0000'ffff);
    TR_ASSERT(t, mmu.IsAddressValid(0x0100'0000'0000'0000));
    TR_ASSERT(t, mmu.IsAddressValid(0x0100'0000'0000'1234));
    TR_ASSERT(t, mmu.IsAddressValid(0x0100'0000'0000'ffff));
    // These are above the defined address range...
    TR_ASSERT(t, !mmu.IsAddressValid(0x0100'0000'0001'0000));
    TR_ASSERT(t, !mmu.IsAddressValid(0x0100'0000'1234'0000));

    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_regionfromaddr(ITesting *t) {
    MMU mmu;
    mmu.Initialize();

    auto region = mmu.RegionFromAddress(0x0100'0000'0000'0000);
    TR_ASSERT(t, region == 0x01);
    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_offsetfromaddr(ITesting *t) {
    MMU mmu;
    mmu.Initialize();

    auto ofs = mmu.PageOffsetFromAddress(0x0000'0000'0000'0123);
    TR_ASSERT(t, ofs == 0x123);

    // 12 bit offsets used, thus 0x1012 = 4096+0x12 => 0x1000+12 => 0x12
    ofs = mmu.PageOffsetFromAddress(0x0000'0000'0000'1012);
    TR_ASSERT(t, ofs == 0x12);

    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_ptefromaddr(ITesting *t) {
    MMU mmu;
    mmu.Initialize();

    uint64_t address = 0x0000'0001'2345'6789;
    auto pte = mmu.PageTableEntryIndexFromAddress(address);
    auto desc = mmu.PageDescriptorIndexFromAddress(address);
    auto tbl = mmu.PageTableIndexFromAddress(address);

    TR_ASSERT(t, tbl == 0x12);
    TR_ASSERT(t, desc = 0x34);
    TR_ASSERT(t, pte = 0x56);

    return kTR_Pass;

}


//
// This copies data to/from external (outside of emulation control) RAM..
//
DLL_EXPORT int test_mmu2_copyext(ITesting *t) {
    RamMemory ramMemory(MMU_MAX_MEM);
    DataBus::Instance().SetRamMemory(&ramMemory);

    MMU mmu;
    mmu.Initialize();
    mmu.SetMMUControl({});

    uint64_t ramMemoryAddr = 0;
    uint32_t writeValue = 0x4711;

    // Start state - all should be invalid..
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);

    mmu.CopyToRamFromExt(ramMemoryAddr, &writeValue, sizeof(writeValue));
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);

    uint32_t readValue = 0;
    mmu.CopyToExtFromRam(&readValue, ramMemoryAddr, sizeof(readValue));
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);
    TR_ASSERT(t, readValue == writeValue);

    return kTR_Pass;
}

// We write a native value into the emulated RAM then we read it...
DLL_EXPORT int test_mmu2_writeext_read(ITesting *t) {
    RamMemory ramMemory(MMU_MAX_MEM);
    DataBus::Instance().SetRamMemory(&ramMemory);

    MMU mmu;
    mmu.Initialize();
    mmu.SetMMUControl({});

    uint64_t ramMemoryAddr = 0;
    uint32_t writeValue = 0x4711;

    // Start state - all should be invalid..
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);

    mmu.CopyToRamFromExt(ramMemoryAddr, &writeValue, sizeof(writeValue));
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);


    // Read to address 64 - this is above the cacheline handling
    uint32_t readValue = mmu.Read<uint32_t>(0);
    TR_ASSERT(t, readValue == writeValue);

    return kTR_Pass;

}

//
// this of write/read through l1 cache
//
DLL_EXPORT int test_mmu2_write_read(ITesting *t) {
    RamMemory ramMemory(MMU_MAX_MEM);
    DataBus::Instance().SetRamMemory(&ramMemory);

    MMU mmu;
    mmu.Initialize();
    mmu.SetMMUControl({});

    uint64_t ramMemoryAddr = 96;
    uint32_t writeValue = 0x4711;

    // Read a raw value directly from RAM - bypassing cache, should be 0
    uint32_t raw;
    mmu.CopyToExtFromRam(&raw, 0, sizeof(raw));
    printf("RAM Before Write: 0x%x\n", raw);

    TR_ASSERT(t, raw == 0);

    // Start state - all should be invalid..
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);

    // Write a 32-bit value, this will pull the memory into the cache and the cache line will be updated
    mmu.Write<uint32_t>(ramMemoryAddr, writeValue);

    // Copy from RAW ram (bypassing cache) - this should still be zero...
    mmu.CopyToExtFromRam(&raw, ramMemoryAddr, sizeof(raw));
    printf("RAM After Write: 0x%x\n", raw);
    TR_ASSERT(t, raw == 0);

    // Read from RAM through cache - this should return the value in the cache
    auto readValue = mmu.Read<uint32_t>(ramMemoryAddr);
    TR_ASSERT(t, readValue == writeValue);
    mmu.GetCacheController().Dump();

    // Copy from RAW ram (bypassing cache) - this should still be zero, as RAM is not yet updated
    mmu.CopyToExtFromRam(&raw, ramMemoryAddr, sizeof(raw));
    TR_ASSERT(t, raw == 0);
    printf("RAW After Read: 0x%x\n", raw);

    mmu.GetCacheController().Dump();

    // Flush the cache to RAM
    mmu.GetCacheController().Flush();

    // Copy from RAW ram (bypassing cache) - we should now have the same value as during the write/read cycle as
    // the cacheline should have been written back to RAM...
    mmu.CopyToExtFromRam(&raw, ramMemoryAddr, sizeof(raw));
    TR_ASSERT(t, raw == writeValue);
    printf("RAW After Flush: 0x%x\n", raw);

    mmu.GetCacheController().Dump();

    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_pagetable_init(ITesting *t) {
    RamMemory ramMemory(MMU_MAX_MEM);
    DataBus::Instance().SetRamMemory(&ramMemory);

    MMU mmu;
    mmu.Initialize();
    mmu.SetMMUControl({kMMU_ResetPageTableOnSet});

    mmu.SetMMUPageTableAddress({0});

    return kTR_Pass;
}


