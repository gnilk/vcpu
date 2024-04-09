//
// Created by gnilk on 08.01.2024.
//

#include <string.h>
#include "MemoryUnit.h"

using namespace gnilk;
using namespace gnilk::vcpu;

//
//
//


// We need a bunch of zeros
static uint8_t empty_cache_line[GNK_L1_CACHE_LINE_SIZE] = {};

void MMU::Initialize(uint8_t newCoreId) {
    coreId = newCoreId;
    cacheController.Initialize(coreId);
}


bool MMU::IsAddressValid(uint64_t address) {
    if (!SoC::Instance().HaveRegionForAddress(address)) {
        return false;
    }
    // Ok, we are good..
    return true;
}


void MMU::SetMMUControl(const RegisterValue &newControl) {
    mmuControl = newControl;
}
void MMU::SetMMUControl(RegisterValue &&newControl) {
    mmuControl = newControl;
}

void MMU::SetMMUPageTableAddress(const gnilk::vcpu::RegisterValue &newPageTblAddr) {
    mmuPageTableAddress = newPageTblAddr;

    if (!IsFlagSet(kMMU_ResetPageTableOnSet)) {
        return;
    }
    if (!IsAddressValid(mmuPageTableAddress.data.longword)) {
        // FIXME: Raise valid exception here
        return;
    }

    // FIXME: need to check if we can do this...

    auto databus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(mmuPageTableAddress.data.longword);

    uint64_t physicalAddr = TranslateAddress(mmuPageTableAddress.data.longword);
    int32_t nBytesToWrite = sizeof(PageTableEntry);

    // Reset everything to zero...
    while(nBytesToWrite) {
        databus->WriteLine(physicalAddr, empty_cache_line);
        nBytesToWrite -= GNK_L1_CACHE_LINE_SIZE;
        if (nBytesToWrite < 0) {
            nBytesToWrite = 0;
            int breakme = 0;
        }

    }

    // Remove the flag..
    mmuControl.data.longword &= ~uint64_t(kMMU_ResetPageTableOnSet);
}

uint64_t MMU::TranslateAddress(uint64_t address) {
    if (!IsFlagSet(kMMU_TranslationEnabled)) {
        return address;
    }
    // Remove the region index
    return address & VCPU_SOC_ADDR_MASK;
}

// Read from RAM => and this interface should go away...
/*
int32_t MMU::Read(uint64_t dstPtr, const uint64_t srcPtr, size_t nBytes) {
    // This is fully bogus...
    if (!IsFlagSet(kMMU_TranslationEnabled)) {
        return cacheController.ReadInternal(dstPtr, srcPtr, nBytes);
    }


    if (!IsAddressValid(dstPtr)) {
        // FIXME: Raise MMU Invalid Address Exception
        return -1;
    }
    if (!IsAddressValid(srcPtr)) {
        // FIXME: Raise MMU Invalid Address Exception
        return -1;
    }

    auto regionDst = RegionFromAddress(dstPtr);
    auto regionSrc = RegionFromAddress(srcPtr);
    if (regionSrc != regionDst) {
        // Inter Region Transfer => DMA
        return -1;
    }


    uint64_t srcAddress = TranslateAddress((uint64_t)srcPtr);
    uint64_t dstAddress = TranslateAddress((uint64_t)dstPtr);
    //
    // We can only cache when
    // 1) cache is enabled (for both src/dst)
    // 2) region is same
    //
    // In case we do inter-regional transfers this is a type of DMA


    // Need to figure out if can/should cache this..
//    if (regions[regionSrc].cbAccessHandler != nullptr) {
//        regions[regionSrc].cbAccessHandler(MesiBusBase::kMemOp::kBusRd, srcAddress);
//    }
//    if (regions[regionDst].cbAccessHandler != nullptr) {
//        regions[regionDst].cbAccessHandler(MesiBusBase::kMemOp::kBusWr, dstAddress);
//    }
    return -1;

}

// Write to RAM
int32_t MMU::Write(uint64_t dstPtr, const uint64_t srcPtr, size_t nBytes) {
    // No translation - addresses are physical...
    uint64_t srcAddress = TranslateAddress((uint64_t)srcPtr);
    uint64_t dstAddress = TranslateAddress((uint64_t)dstPtr);
    return cacheController.WriteInternal(dstAddress, srcAddress, nBytes);
    return -1;
}
 */

int32_t MMU::WriteInternalFromExternal(uint64_t address, const void *src, size_t nBytes) {
    // FIXME: Address translation
    return cacheController.WriteInternalFromExternal(address, src, nBytes);
}
void MMU::ReadInternalToExternal(void *dst, uint64_t address, size_t nBytes) {
    // FIXME: Address translation
    cacheController.ReadInternalToExternal(dst, address, nBytes);
}


// Copy to external (native) RAM from the emulated RAM...
int32_t MMU::CopyToExtFromRam(void *dstPtr, const uint64_t srcAddress, size_t nBytes) {
    static uint8_t tmpCacheLine[GNK_L1_CACHE_LINE_SIZE];
    if (nBytes > GNK_L1_CACHE_LINE_SIZE) {
        return -1;
    }
    if (nBytes == 0) {
        return 0;
    }
    if (!SoC::Instance().HaveRegionForAddress(srcAddress)) {
        // FIXME: Raise exception
        return 0;
    }

    // FIXME: Verify if we can cahce this region - if not - we should fetch another bus!!


    auto databus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(srcAddress);
    memset(tmpCacheLine, 0, GNK_L1_CACHE_LINE_SIZE);
    databus->ReadLine(tmpCacheLine, srcAddress);
    memcpy(dstPtr, tmpCacheLine, nBytes);
    return nBytes;
}


// Copy to emulated RAM from native RAM...
int32_t MMU::CopyToRamFromExt(uint64_t dstAddr, const void *srcAddress, size_t nBytes) {
    static uint8_t tmpCacheLine[GNK_L1_CACHE_LINE_SIZE];
    if (nBytes > GNK_L1_CACHE_LINE_SIZE) {
        return -1;
    }
    if (nBytes == 0) {
        return 0;
    }
    if (!SoC::Instance().HaveRegionForAddress(dstAddr)) {
        // FIXME: Raise exception
        return 0;
    }

    // FIXME: Verify if we can cahce this region - if not - we should fetch another bus!!

    auto databus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(dstAddr);

    memset(tmpCacheLine, 0, GNK_L1_CACHE_LINE_SIZE);
    memcpy(tmpCacheLine, srcAddress, nBytes);
    databus->WriteLine(dstAddr, tmpCacheLine);

    return nBytes;
}





/*










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
    size_t minMemRequired = sizeof(PageTableEntry) + ((VCPU_MMU_MAX_ROOT_TABLES * VCPU_MMU_MAX_DESCRIPTORS * VCPU_MMU_MAX_PAGES_PER_DESC) >> 3);

    if (sizeInBytes < minMemRequired) {
        // FIXME: Raise low-memory exception - we need AT LEAST some bytes
        return false;
    }

    ptrPhysicalRamStart = physicalRam;
    szPhysicalRam = sizeInBytes;
    ptrFirstAvailablePage = static_cast<uint8_t *>(ptrPhysicalRamStart);

    phyBitMap = static_cast<uint8_t *>(ptrPhysicalRamStart);
    szBitmapBytes = (VCPU_MMU_MAX_ROOT_TABLES * VCPU_MMU_MAX_DESCRIPTORS * VCPU_MMU_MAX_PAGES_PER_DESC) >> 3;

    ptrFirstAvailablePage += szBitmapBytes;

    InitializePhysicalBitmap();

    rootTables = nullptr;
    return true;
}

// This will replace the mmu control register
void MemoryUnit::UpdateMMUControl(const RegisterValue &newMMUControl) {
//    bool bWasEnabled = mmuControl.data.longword & kMMUEnable_Translation;
    mmuControl = newMMUControl;
//    if (bWasEnabled) {
//
//    }
}

void MemoryUnit::SetPageTableAddress(const RegisterValue &newVirtualPageTableAddress) {
    mmuPageTableAddress = newVirtualPageTableAddress;

    if (IsFlagEnabled(kMMU_ResetPageTableOnSet)) {
        // This will be a pass-through translation if translation is not enabled..
        auto ptrPhysicalAddrTable = TranslateToPhysical(mmuPageTableAddress.data.longword);
        memset(ptrPhysicalAddrTable, 0, sizeof(PageTableEntry));

        // Remove the flag..
        mmuControl.data.longword &= ~uint64_t(kMMU_ResetPageTableOnSet);
    }
}

// Internal, initialize the root tables in the current physical RAM
void MemoryUnit::InitializePhysicalBitmap() {
    memset(phyBitMap,0, szBitmapBytes);

}

PageTableEntry *MemoryUnit::GetPageTables() const {
    return (PageTableEntry *)mmuPageTableAddress.data.nativePtr;
}


// Returns the page-table for a virtual address, or null if none
// this is for debugging and unit-testing purposes...
const Page *MemoryUnit::GetPageForAddress(uint64_t virtualAddress) {
    uint64_t idxRootTable = IdxRootTableFromVA(virtualAddress);
    uint64_t idxDesc = IdxDescriptorFromVA(virtualAddress);
    uint64_t idxPage = IdxPageFromVA(virtualAddress);

    if (rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage].flags == 0x00) {
        return nullptr;
    }

    return &rootTables[idxRootTable].descriptor[idxDesc].pages[idxPage];
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
    // 1) TLB Lookup, hit -> protection check
    // 2) Page table walk -> not resident in memory -> Page Fault Handler -> OS/Handler load page
    // 3) Update TLB
    // 4) Protection check, denied -> Protection Fault
    // 5) Physical address [update cache]

    // In case no translation - just return back the virtual address...
    if (!IsFlagEnabled(kMMU_TranslationEnabled)) {
        // Mask out reserved bits from this virtual address and return it..
        return (virtualAddress & VCPU_SOC_ADDR_MASK);
    }

    auto page = GetPageForAddress(virtualAddress);
    if (page == nullptr) {
        // fixme: raise mmu-exception page-no-found
        return 0;
    }

    return virtualAddress;
}

// This assumes the host is 64bit
void *MemoryUnit::TranslateToPhysical(uint64_t virtualAddress) {
    auto phyAddress = TranslateAddress(virtualAddress);
    return (void *)(phyAddress);
}

void *MemoryUnit::AllocatePhysicalPage(uint64_t virtualAddress) const {
    // Remove the VADDR start bit..  This is artifically added to the address..
    virtualAddress &= ~MemoryUnit::VIRTUAL_ADDR_SPACE_START;

    auto bitmapIndex = (virtualAddress>>12) >> 3;
    auto pageBit = (virtualAddress>>12) & 7;

    // Note: the physical bitmap table must be in globally shared RAM...
    if (phyBitMap[bitmapIndex] & (1 << pageBit)) {
        // Page is occupied, we need to start a search...
        int breakme = 1;
    }

    phyBitMap[bitmapIndex] |= (1 << pageBit);

    // physical memory, perhaps just '>>12'
    auto phyPageIndex = (virtualAddress>>12);
    auto ptrPhysical = &ptrFirstAvailablePage[phyPageIndex * VCPU_MMU_PAGE_SIZE];

//    auto *ptr = ptrCurrentPage;
//    ptrCurrentPage += szPhysicalRam;
    return ptrPhysical;
}

void MemoryUnit::FreePhysicalPage(uint64_t virtualAddress) const {
    // Remove the VADDR start bit..  This is artifically added to the address..
    virtualAddress &= ~MemoryUnit::VIRTUAL_ADDR_SPACE_START;

    auto bitmapIndex = (virtualAddress>>12) >> 3;
    auto pageBit = (virtualAddress>>12) & 7;
    if (!(phyBitMap[bitmapIndex] & (1 << pageBit))) {
        // Page is not occupied, trying to free already freed or not allocated memory
        int breakme = 1;
        return;
    }
    phyBitMap[bitmapIndex] = phyBitMap[bitmapIndex] & ((1<<pageBit) ^ 0xff);
}

*/