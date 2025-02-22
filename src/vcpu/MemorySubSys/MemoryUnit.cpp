//
// Created by gnilk on 08.01.2024.
//

#include <string.h>
#include "MemoryUnit.h"
#include "FlashBus.h"
#include "System.h"

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

    auto databus = SoC::Instance().GetDataBusForAddress(mmuPageTableAddress.data.longword);

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
    return address & VCPU_MEM_ADDR_MASK;
}

void MMU::Touch(const uint64_t address) {
    // Can't cache this - don't try...
    if (!SoC::Instance().IsAddressCacheable(address)) {
        return;
    }
    cacheController.Touch(address);
}

int32_t MMU::WriteInternalFromExternal(uint64_t virtualAddress, const void *src, size_t nBytes) {
    // FIXME: Address translation
    if (!SoC::Instance().IsAddressCacheable(virtualAddress)) {
        auto bus = SoC::Instance().GetDataBusForAddress(virtualAddress);
        if (bus == nullptr) {
            return -1;
        }
        // FIXME: The non-cache-able bus should work on smaller values - 32bit?
        bus->WriteData(virtualAddress, src, nBytes);
        return (int32_t)nBytes;
    }

    return cacheController.WriteInternalFromExternal(virtualAddress, src, nBytes);
}
void MMU::ReadInternalToExternal(void *dst, uint64_t virtualAddress, size_t nBytes) {
    // FIXME: Address translation
    if (!SoC::Instance().IsAddressCacheable(virtualAddress)) {
        auto bus = SoC::Instance().GetDataBusForAddress(virtualAddress);
        if (bus == nullptr) {
            return;
        }
        bus->ReadData(dst, virtualAddress, nBytes);
        return;
    }
    cacheController.ReadInternalToExternal(dst, virtualAddress, nBytes);
}


// Copy to external (native) RAM from the emulated RAM...
int32_t MMU::CopyToExtFromRam(void *dstPtr, const uint64_t srcVirtualAddress, size_t nBytes) {
    if (nBytes == 0) {
        return 0;
    }
    if (!SoC::Instance().HaveRegionForAddress(srcVirtualAddress)) {
        // FIXME: Raise exception
        return 0;
    }

    auto ram = SoC::Instance().GetFirstRegionFromBusType<RamBus>();
    if (ram == nullptr) {
        // FIXME: Raise exception
        return -1;
    }
    if (ram->bus == nullptr) {
        // FIXME: Raise exception
        return -1;
    }
    // Check if we are within the region..
    if ((srcVirtualAddress < ram->vAddrStart) || (srcVirtualAddress > ram->vAddrEnd)) {
        // FIXME: Raise exception here - invalid address
        return -1;
    }
    if ((srcVirtualAddress + nBytes) > ram->vAddrEnd) {
        // FIXME: Raise exception here - copy-outside of available RAM - PageFault
        return -1;
    }

    // Make sure we flush the cache first!!!
    cacheController.Flush();

    auto ramAddress = TranslateAddress(srcVirtualAddress);
    ram->bus->ReadData(dstPtr, ramAddress, nBytes);

    return nBytes;
}


// Copy to emulated RAM from native RAM...
int32_t MMU::CopyToRamFromExt(uint64_t dstVirtualAddr, const void *srcAddress, size_t nBytes) {
    if (nBytes == 0) {
        return 0;
    }
    if (!SoC::Instance().HaveRegionForAddress(dstVirtualAddr)) {
        // FIXME: Raise exception
        return 0;
    }

    auto ram = SoC::Instance().GetFirstRegionFromBusType<RamBus>();
    if (ram == nullptr) {
        // FIXME: Raise exception
        return -1;
    }
    if (ram->bus == nullptr) {
        // FIXME: Raise exception
        return -1;
    }
    // Check if we are within the region..
    if ((dstVirtualAddr < ram->vAddrStart) || (dstVirtualAddr > ram->vAddrEnd)) {
        // FIXME: Raise exception here - invalid address
        return -1;
    }
    if ((dstVirtualAddr + nBytes) > ram->vAddrEnd) {
        // FIXME: Raise exception here - copy-outside of available RAM - PageFault
        return -1;
    }

    // Make sure we flush the cache first!!!
    cacheController.Flush();

    auto ramAddress = TranslateAddress(dstVirtualAddr);
    ram->bus->WriteData(ramAddress, srcAddress, nBytes);

    return nBytes;
}





/*

the old MMU is down below - I started down the wrong lane and did a whole bunch of not-yet-needed stuff.
I will rip code out of here before I take it away. This had proper page-walking and page-allocation handling
 which is not yet in the new MMU (focus was memory access).








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