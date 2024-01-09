//
// Created by gnilk on 08.01.2024.
//

#include "MemoryUnit.h"

using namespace gnilk;
using namespace gnilk::vcpu;

// Initialize MMU and the page-table directory
bool MemoryUnit::Initialize(void *physicalRam, size_t sizeInBytes) {
    ptrPhysicalRamStart = physicalRam;
    szPhysicalRam = sizeInBytes;
    ptrCurrentPage = static_cast<uint8_t *>(ptrPhysicalRamStart);

    // Allocate one page table for the mmu on initialization
    rootTables = (PageTable *)physicalRam;
    InitializeRootTables();

    ptrCurrentPage += sizeof(PageTable);

    return true;
}

// Internal, initialize the root tables in the current physical RAM
void MemoryUnit::InitializeRootTables() {
    for(int i=0;i<VCPU_MMU_MAX_ROOT_TABLES;i++) {
        rootTables[i].nDescriptors = 0;
        for(int idxDesc=0;idxDesc<VCPU_MMU_MAX_DESCRIPTORS;idxDesc++) {
            rootTables[i].descriptor[idxDesc].nPages = 0;
            for(int idxPage=0; idxPage<VCPU_MMU_MAX_PAGES_PER_DESC; idxPage++) {
                rootTables[i].descriptor[idxDesc].pages[idxPage].ptrPhysical = nullptr;
                rootTables[i].descriptor[idxDesc].pages[idxPage].bytes = 0;
                rootTables[i].descriptor[idxDesc].pages[idxPage].flags = 0;
            }
        }
    }
}

// Allocates a page from physical RAM
uint64_t MemoryUnit::AllocatePage() {
    return AllocateLargePage(VCPU_MMU_PAGE_SIZE);
}

uint64_t MemoryUnit::AllocateLargePage(size_t szPage) {

    int64_t idxRootTable = FindFreeRootTable();
    if (idxRootTable < 0) {
        return 0;
    }
    int64_t idxDesc = FindFreePageDescriptor(idxRootTable);
    if (idxDesc < 0) {
        return 0;
    }
    int64_t idxPage = FindFreePage(idxRootTable, idxDesc);
    if (idxPage < 0) {
        return 0;
    }

    // Indicate number of pages, and allocate one..
    rootTables[idxRootTable].descriptor[idxDesc].nPages++;
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].bytes = szPage;
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].ptrPhysical = AllocatePhysicalPage(szPage);
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags = MMU_FLAGS_RWX;

    // 12 = 4k per page..
    uint64_t virtualRoot = (idxRootTable<<(12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS));
    uint64_t virtualDesc = (idxDesc << (12 + VCPU_MMU_PAGES_NBITS));
    uint64_t viurtualPage = (idxPage << 12);

    uint64_t virtualAddress = virtualRoot | virtualDesc | viurtualPage;

    return virtualAddress;
}

// Returns the page-table for a virtual address, or null if none
// this is for debugging and unit-testing purposes...
const MemoryUnit::Page *MemoryUnit::GetPageForAddress(uint64_t virtualAddress) {
    uint64_t idxRootTable = virtualAddress >> (12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS);
    uint64_t idxDesc = virtualAddress >> (12 + VCPU_MMU_PAGES_NBITS);
    uint64_t idxPage = virtualAddress >> (12);

    if (rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags == 0x00) {
        return nullptr;
    }

    return &rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage];
}

//
// I probably need to optimize this quite a bit - have some reading to do in order to figure out a
// FPGA/HW friendly way to do this...
//

// Finds a free root table...
int64_t MemoryUnit::FindFreeRootTable() {
    for(int i=0;i<VCPU_MMU_MAX_ROOT_TABLES;i++) {
        // Must be -1 since we will allocate this free slot..
        if (rootTables[i].nDescriptors < (VCPU_MMU_MAX_DESCRIPTORS-1)) {
            return i;
        }
    }
    return -1;
}

// Finds a free page-descriptor within the root table..
int64_t MemoryUnit::FindFreePageDescriptor(uint64_t idxRootTable) {
    for(int i=0;i<VCPU_MMU_MAX_DESCRIPTORS;i++) {
        // Must be -1 since we will allocate this free slot..
        if (rootTables[idxRootTable].descriptor[i].nPages < (VCPU_MMU_MAX_PAGES_PER_DESC-1)) {
            return i;
        }
    }
    return -1;
}

// Finds a free page within the descriptor table...
int64_t MemoryUnit::FindFreePage(uint64_t idxRootTable, uint64_t idxDescriptor) {
    for(int i=0;i<VCPU_MMU_MAX_PAGES_PER_DESC;i++) {
        if (rootTables[idxRootTable].descriptor[idxDescriptor].pages[i].flags == 0x00) {
            return i;
        }
    }
    return -1;
}

// Checks if a specific virtual address is valid - basically checks if there is a
bool MemoryUnit::IsAddressValid(uint64_t virtualAddress) {
    uint64_t idxRootTable = virtualAddress >> (12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS);
    uint64_t idxDesc = virtualAddress >> (12 + VCPU_MMU_PAGES_NBITS);
    uint64_t idxPage = virtualAddress >> (12);

    // Dont't think the following is needed - as we are guranteed to look within boundaries
    // if (rootTables[idxRootTable].nDescriptors <= idxDesc) {
    //     return false;
    // }
    // if (rootTables[idxRootTable].descriptor[idxDesc].nPages <= idxPage) {
    //     return false;
    // }
    if (rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags == 0) {
        return false;
    }

    return true;
}
uint64_t MemoryUnit::TranslateAddress(uint64_t virtualAddress) {
    return virtualAddress;
}


void *MemoryUnit::AllocatePhysicalPage(size_t szPage) {
    auto *ptr = ptrCurrentPage;
    ptrCurrentPage += szPhysicalRam;
    return ptr;

}
