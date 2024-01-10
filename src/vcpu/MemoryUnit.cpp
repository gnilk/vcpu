//
// Created by gnilk on 08.01.2024.
//

#include "MemoryUnit.h"

using namespace gnilk;
using namespace gnilk::vcpu;

//
// Hmm, not sure this should all be part of the MMU or IF some of this should be placed at the 'kernel' / 'bios' level
// The MMU should basically just translate based on tables defined by the underlying system..
// Thus, the 'AllocPage' / 'FreePage' handling should be moved out of here...
// Instead the MMU should have two registers (variables):
//  Control - defines the operational mode of the MMU translation unit
//  PageTableAddr - defines the PageTable address currently being used..
//
//

// Initialize MMU and the page-table directory
bool MemoryUnit::Initialize(void *physicalRam, size_t sizeInBytes) {

    // Minimum memory required to hold our data..
    // for a 16x16x16 config this is about 1kb (1041 bytes)...
    // This the amount of RAM we steal to hold our meta-data
    size_t minMemRequired = sizeof(PageTable) + (VCPU_MMU_MAX_ROOT_TABLES * VCPU_MMU_MAX_DESCRIPTORS * VCPU_MMU_MAX_PAGES_PER_DESC) >> 3;

    if (sizeInBytes < minMemRequired) {
        // FIXME: Raise low-memory exception - we need AT LEAST some bytes
        return false;
    }


    ptrPhysicalRamStart = physicalRam;
    szPhysicalRam = sizeInBytes;
    ptrCurrentPage = static_cast<uint8_t *>(ptrPhysicalRamStart);

    phyBitMap = static_cast<uint8_t *>(ptrPhysicalRamStart);
    szBitmapBytes = (VCPU_MMU_MAX_ROOT_TABLES * VCPU_MMU_MAX_DESCRIPTORS * VCPU_MMU_MAX_PAGES_PER_DESC) >> 3;

    ptrCurrentPage += szBitmapBytes;

    // Allocate one page table for the mmu on initialization
    rootTables = (PageTable *)ptrCurrentPage;
    InitializeRootTables();

    ptrCurrentPage += sizeof(PageTable);

    return true;
}

// Internal, initialize the root tables in the current physical RAM
void MemoryUnit::InitializeRootTables() {
    // for(int i=0;i<szBitmapBytes;i++) {
    //     phyBitMap[i] = 0;
    // }
    memset(phyBitMap,0, szBitmapBytes);
    // This replaces the whole loop stuff
    memset(rootTables, 0, sizeof(PageTable));


/*
 * leaving this for the time beeing..
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
*/
}

// Allocates a page from physical RAM
uint64_t MemoryUnit::AllocatePage() {

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

    // 12 = 4k per page..
    uint64_t virtualRoot = (idxRootTable<<(12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS));
    uint64_t virtualDesc = (idxDesc << (12 + VCPU_MMU_PAGES_NBITS));
    uint64_t viurtualPage = (idxPage << 12);

    uint64_t virtualAddress = virtualRoot | virtualDesc | viurtualPage;

    // Indicate number of pages, and allocate one..
    rootTables[idxRootTable].descriptor[idxDesc].nPages++;
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].bytes = VCPU_MMU_PAGE_SIZE;
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].ptrPhysical = AllocatePhysicalPage(virtualAddress);
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags = MMU_FLAGS_RWX;

    return virtualAddress;
}

bool MemoryUnit::FreePage(uint64_t virtualAddress) {
    uint64_t idxRootTable = IdxRootTableFromVA(virtualAddress);
    uint64_t idxDesc = IdxDescriptorFromVA(virtualAddress);
    uint64_t idxPage = IdxPageFromVA(virtualAddress);

    if (rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags == 0x00) {
        return false;
    }

    FreePhysicalPage(virtualAddress);

    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags = 0;
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].bytes = 0;
    rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].ptrPhysical = nullptr;
    rootTables[idxRootTable].descriptor[idxDesc].nPages -= 1;

    return true;
}

// Returns the page-table for a virtual address, or null if none
// this is for debugging and unit-testing purposes...
const MemoryUnit::Page *MemoryUnit::GetPageForAddress(uint64_t virtualAddress) {
    uint64_t idxRootTable = IdxRootTableFromVA(virtualAddress);
    uint64_t idxDesc = IdxDescriptorFromVA(virtualAddress);
    uint64_t idxPage = IdxPageFromVA(virtualAddress);

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
        if (rootTables[idxRootTable].descriptor[i].nPages < VCPU_MMU_MAX_PAGES_PER_DESC) {
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
    uint64_t idxRootTable = IdxRootTableFromVA(virtualAddress);
    uint64_t idxDesc = IdxDescriptorFromVA(virtualAddress);
    uint64_t idxPage = IdxPageFromVA(virtualAddress);

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

// FIXME: Need a way to track these
void *MemoryUnit::AllocatePhysicalPage(uint64_t virtualAddress) {

    auto bitmapIndex = (virtualAddress>>12) >> 3;
    auto pageBit = (virtualAddress>>12) & 7;
    if (phyBitMap[bitmapIndex] & (1 << pageBit)) {
        // Page is occupied, we need to start a search...
        int breakme = 1;
    }

    phyBitMap[bitmapIndex] |= (1 << pageBit);

    // physical memory, perhaps just '>>12'
    auto phyPageIndex = (virtualAddress>>12);
    auto ptrPhysical = &ptrCurrentPage[phyPageIndex * VCPU_MMU_PAGE_SIZE];

//    auto *ptr = ptrCurrentPage;
//    ptrCurrentPage += szPhysicalRam;
    return ptrPhysical;
}

void MemoryUnit::FreePhysicalPage(uint64_t virtualAddress) {
    // FIXME: Implement this
    auto bitmapIndex = (virtualAddress>>12) >> 3;
    auto pageBit = (virtualAddress>>12) & 7;
    if (!(phyBitMap[bitmapIndex] & (1 << pageBit))) {
        // Page is not occupied, trying to free already freed or not allocated memory
        int breakme = 1;
        return;
    }
    phyBitMap[bitmapIndex] = phyBitMap[bitmapIndex] & ((1<<pageBit) ^ 0xff);
}

