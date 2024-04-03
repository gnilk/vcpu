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
DLL_EXPORT int test_mmu2_copyext(ITesting *t);
DLL_EXPORT int test_mmu2_writeext_read(ITesting *t);
DLL_EXPORT int test_mmu2_write_read(ITesting *t);
DLL_EXPORT int test_mmu2_pagetable_init(ITesting *t);
}


DLL_EXPORT int test_mmu2(ITesting *t) {
    return kTR_Pass;
}

DLL_EXPORT int test_mmu2_copyext(ITesting *t) {
    RamMemory ramMemory(MMU_MAX_MEM);
    DataBus::Instance().SetRamMemory(&ramMemory);

    MMU mmu;
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

DLL_EXPORT int test_mmu2_write_read(ITesting *t) {
    RamMemory ramMemory(MMU_MAX_MEM);
    DataBus::Instance().SetRamMemory(&ramMemory);

    MMU mmu;
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
    mmu.SetMMUControl({kMMU_ResetPageTableOnSet});

    mmu.SetMMUPageTableAddress({0});

    return kTR_Pass;
}


