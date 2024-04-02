//
// Created by gnilk on 02.04.2024.
//

#include "PageAllocator.h"

using namespace gnilk;
using namespace gnilk::vcpu;

uint64_t PageAllocator::AllocatePage(const MemoryUnit &mmu) {

    // The page allocation handling is only used to track virtual to physical address space mapping of the 4k pages
    int64_t idxRootTable = FindFreeRootTable(mmu);
    if (idxRootTable < 0) {
        return 0;
    }
    int64_t idxDesc = FindFreePageDescriptor(mmu, idxRootTable);
    if (idxDesc < 0) {
        return 0;
    }
    int64_t idxPage = FindFreePage(mmu, idxRootTable, idxDesc);
    if (idxPage < 0) {
        return 0;
    }

    // 12 = 4k per page..

    //
    // This translation belongs in the MMU
    //
    uint64_t virtualRoot = (idxRootTable<<(12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS));
    uint64_t virtualDesc = (idxDesc << (12 + VCPU_MMU_PAGES_NBITS));
    uint64_t viurtualPage = (idxPage << 12);

    uint64_t virtualAddress = (virtualRoot | virtualDesc | viurtualPage) | MemoryUnit::VIRTUAL_ADDR_SPACE_START;


    auto phyAddress = mmu.AllocatePhysicalPage(virtualAddress);


    auto pageTables = mmu.GetPageTables();
    // Indicate number of pages, and allocate one..
    pageTables[idxRootTable].descriptor[idxDesc].nPages++;
    pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].bytes = VCPU_MMU_PAGE_SIZE;
    pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags = MMU_FLAGS_RWX;

    // address translation enabled, store the physical address
    if (mmu.IsFlagEnabled(kMMU_TranslationEnabled)) {
        pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].ptrPhysical = phyAddress;
        return virtualAddress;
    }

    // If translation is disabled we store the virtual address...
    pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].ptrPhysical = (void *)virtualAddress;
    return (uint64_t)phyAddress;
}

bool PageAllocator::FreePage(const MemoryUnit &mmu, uint64_t virtualAddress) {
    if (!mmu.IsFlagEnabled(kMMU_TranslationEnabled)) {
        // FIXME: Not sure what a good way - I can blow up the page-tables - or not..
        // The MMU currently have the 'bitmap' table - is this a good idea??
        return true;
    }
    auto pageTables = mmu.GetPageTables();



    uint64_t idxRootTable = mmu.IdxRootTableFromVA(virtualAddress);
    uint64_t idxDesc = mmu.IdxDescriptorFromVA(virtualAddress);
    uint64_t idxPage = mmu.IdxPageFromVA(virtualAddress);

    if (pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags == 0x00) {
        return false;
    }

    mmu.FreePhysicalPage(virtualAddress);

    pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags = 0;
    pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].bytes = 0;
    pageTables[idxRootTable].descriptor[idxDesc].pages[idxPage].ptrPhysical = nullptr;
    pageTables[idxRootTable].descriptor[idxDesc].nPages -= 1;

    return true;
}

//
// I probably need to optimize this quite a bit - have some reading to do in order to figure out a
// FPGA/HW friendly way to do this...
//

// Finds a free root table...
int64_t PageAllocator::FindFreeRootTable(const MemoryUnit &mmu) {
    auto pageTables = mmu.GetPageTables();

    for(int i=0;i<VCPU_MMU_MAX_ROOT_TABLES;i++) {
        // Must be -1 since we will allocate this free slot..
        if (pageTables[i].nDescriptors < (VCPU_MMU_MAX_DESCRIPTORS-1)) {
            return i;
        }
    }
    return -1;
}

// Finds a free page-descriptor within the root table..
int64_t PageAllocator::FindFreePageDescriptor(const MemoryUnit &mmu, uint64_t idxRootTable) {
    auto pageTables = mmu.GetPageTables();
    for(int i=0;i<VCPU_MMU_MAX_DESCRIPTORS;i++) {
        // Must be -1 since we will allocate this free slot..
        if (pageTables[idxRootTable].descriptor[i].nPages < VCPU_MMU_MAX_PAGES_PER_DESC) {
            return i;
        }
    }
    return -1;
}

// Finds a free page within the descriptor table...
int64_t PageAllocator::FindFreePage(const MemoryUnit &mmu, uint64_t idxRootTable, uint64_t idxDescriptor) {
    auto pageTables = mmu.GetPageTables();
    for(int i=0;i<VCPU_MMU_MAX_PAGES_PER_DESC;i++) {
        if (pageTables[idxRootTable].descriptor[idxDescriptor].pages[i].flags == 0x00) {
            return i;
        }
    }
    return -1;
}
