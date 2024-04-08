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
    mmu.Read(64,0,sizeof(writeValue));
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() != GNK_L1_CACHE_NUM_LINES);

    // Now read back from this address
    uint32_t readValue = 0;
    mmu.CopyToExtFromRam(&readValue, 64, sizeof(readValue));
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

    uint64_t ramMemoryAddr = 0;
    uint32_t writeValue = 0x4711;
    uint32_t writeValueEmu = 0x1234;

    // Start state - all should be invalid..
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);

    // put 0x4711 => emulated_ram[0]
    mmu.CopyToRamFromExt(ramMemoryAddr, &writeValue, sizeof(writeValue));
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() == GNK_L1_CACHE_NUM_LINES);

    // emulated_ram[64] <= emulated_ram[0]

    // Read to address 64 - this is above the cacheline handling
    mmu.Read(64,0,sizeof(writeValue));
    TR_ASSERT(t, mmu.GetCacheController().GetInvalidLineCount() != GNK_L1_CACHE_NUM_LINES);
    printf("After 'Read' to address 64 from address 0\n");
    mmu.GetCacheController().Dump();

    // Now read back from this address
    uint32_t readValue = 0;
    mmu.CopyToExtFromRam(&readValue, 64, sizeof(readValue));
    TR_ASSERT(t, readValue == writeValue);

    readValue = 0x1234;
    mmu.CopyToRamFromExt(64, &readValue, sizeof(readValue));

    mmu.Write(0, 64, sizeof(readValue));
    printf("After write to 0 from 64\n");
    mmu.GetCacheController().Dump();
    // Issue a read - so we change the state...
    mmu.Read(128,0, sizeof(readValue));
    printf("After Read to 128 from 0\n");

    mmu.GetCacheController().Flush();

    uint32_t readValue2 = 0;
    mmu.CopyToExtFromRam(&readValue2, 0, sizeof(readValue));
    TR_ASSERT(t, readValue2 == 0x1234);

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


