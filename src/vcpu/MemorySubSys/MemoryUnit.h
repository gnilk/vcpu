//
// Created by gnilk on 08.01.2024.
//

#ifndef MEMORYUNIT_H
#define MEMORYUNIT_H

#include <stdlib.h>
#include <stdint.h>
#include <unordered_map>
#include "CPUMemCache.h"
#include "RegisterValue.h"

//
// When refactoring take a look at RISC-V, https://github.com/riscv-software-src/riscv-isa-sim
// And: https://ics.uci.edu/~swjun/courses/2021W-CS152/slides/lec10%20-%20Virtual%20memory.pdf
// Specifically slides 8.. and then 18..
// Pay care to slide 15 and 16 (TLB)
// Also decide if cache comes before TLB or after (see slides 25..30)
//
// Move this to 'MemorySubSys' and integrate with CPUMemCache
//
// Slide 39 provides a nice overview of HW/SW
//
// When allocating a page we need to make sure that 'VirtualRAM' doesn't start at address 0
// Like we can reserve the first 4gb of virtual memory to something else...
//
//
namespace gnilk {
    namespace vcpu {
        // DO NOT CHANGE
        static const size_t VCPU_MMU_MAX_ROOT_TABLES = 16;
        static const size_t VCPU_MMU_ROOT_TABLES_NBITS = 4;        // Should be the bit position of 16

        static const size_t VCPU_MMU_MAX_DESCRIPTORS = 16;
        static const size_t VCPU_MMU_DESCRIPTORS_NBITS = 4;

        static const size_t VCPU_MMU_MAX_PAGES_PER_DESC = 16;
        static const size_t VCPU_MMU_PAGES_NBITS = 4;

        static const size_t VCPU_MMU_PAGE_SIZE = 4096;

        // Page table flags...
        static const uint32_t MMU_FLAG_READ =  0x01;    // allow read
        static const uint32_t MMU_FLAG_WRITE = 0x02;    // allow write
        static const uint32_t MMU_FLAG_EXEC =  0x04;    // allow execute

        static const uint32_t MMU_FLAGS_RWX =  0x07;    // allow Read|Write|Exec - this is the default..

        enum kMMUFlagsCR0 {
            kMMU_TranslationEnabled = 1,
            kMMU_ResetPageTableOnSet = 2,
        };

#pragma pack(push,1)
        // 16 bytes
        struct Page {
            void *ptrPhysical = nullptr;    // 64 bit?
            uint32_t bytes = 0;             // 32 bit, page size - max 4gb continous memory
            uint32_t flags = 0;  // TBD     // 32 bit
        };
        // 16 * VCPU_MMU_MAX_PAGES_PER_DESC + 8
        struct PageDescriptor {
            size_t nPages = 0;  // need to remove this - must have better structure!
            Page pages[VCPU_MMU_MAX_PAGES_PER_DESC];
        };
        // VCPU_MMU_MAX_DESCRIPTORS * 16 * VCPU_MMU_MAX_PAGES_PER_DESC + 8 + 8
        struct PageTable {
            size_t nDescriptors = 0;    // need to remove this - must have better structure
            PageDescriptor descriptor[VCPU_MMU_MAX_DESCRIPTORS];
        };
#pragma pack(pop)



        // New version
        class MMU {
        public:
            MMU() = default;
            virtual ~MMU() = default;

            void SetMMUControl(const RegisterValue &newControl);
            void SetMMUControl(RegisterValue &&newControl);
            void SetMMUPageTableAddress(const RegisterValue &newPageTblAddr);

            // Emulated RAM <-> RAM transfers, with cache line handling
            int32_t Read(uint64_t dstAddress, const uint64_t srcAddress, size_t nBytes);
            int32_t Write(uint64_t dstAddress, const uint64_t srcAddress, size_t nBytes);


            // External RAM <-> Emulated RAM functions - this will stall the bus!
            // Read to external address from emulated RAM
            int32_t CopyToExtFromRam(void *dstPtr, const uint64_t srcAddress, size_t nBytes);
            // Write to emulated RAM from external address (native)
            int32_t CopyToRamFromExt(uint64_t dstAddr, const void *srcAddress, size_t nBytes);


            uint64_t TranslateAddress(uint64_t address);

            __inline bool IsFlagSet(kMMUFlagsCR0 flag) const {
                return (mmuControl.data.longword & flag);
            }

            const CacheController &GetCacheController() const {
                return cacheController;
            }
            CacheController &GetCacheController() {
                return cacheController;
            }
        protected:

        protected:
            RegisterValue mmuControl;
            RegisterValue mmuPageTableAddress;
            CacheController cacheController;
        };



        // addresses are always 64bit, a mapped pages is looked up like:
        //
        // [8][8][8][....][12] =>
        //  8 bit table-index
        //  8 bit descriptor-index
        //  8 bit page-index
        //  [12 bits unused]
        //  12 bit page-offset

        class MemoryUnit {
        public:
            // This is where the virtual address space starts - everything below this is reserved!
            static const uint64_t VIRTUAL_ADDR_SPACE_START = 0x80000000;
        public:
            MemoryUnit() = default;
            virtual ~MemoryUnit() = default;


            void UpdateMMUControl(const RegisterValue &mmuCntrl);
            void SetPageTableAddress(const RegisterValue &newVirtualPageTableAddress);


            bool Initialize(void *physicalRam, size_t sizeInBytes);
            bool IsAddressValid(uint64_t virtualAddress);

            PageTable *GetPageTables() const;

            uint64_t TranslateAddress(uint64_t virtualAddress);
            // debugging / unit-test functionality
            const Page *GetPageForAddress(uint64_t virtualAddress);

            const uint8_t *GetPhyBitmapTable() {
                return phyBitMap;
            }
            size_t GetPhyBitmapTableSize() {
                return szBitmapBytes;
            }

            inline uint64_t IdxRootTableFromVA(uint64_t virtualAddress) const {
                return (virtualAddress >> (12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS)) & (VCPU_MMU_MAX_ROOT_TABLES-1);
            }
            inline uint64_t IdxDescriptorFromVA(uint64_t virtualAddress) const {
                return (virtualAddress >> (12 + VCPU_MMU_PAGES_NBITS)) & (VCPU_MMU_MAX_DESCRIPTORS - 1);
            }
            inline uint64_t IdxPageFromVA(uint64_t virtualAddress) const {
                return (virtualAddress >> 12) & (VCPU_MMU_MAX_PAGES_PER_DESC-1);
            }


            void *AllocatePhysicalPage() const;
            void FreePhysicalPage(void *ptrPhysical) const;

            void *AllocatePhysicalPage(uint64_t virtualAddress) const;
            void FreePhysicalPage(uint64_t virtualAddress) const;

            __inline bool IsFlagEnabled(kMMUFlagsCR0 flag) const {
                return (mmuControl.data.longword & flag);
            }

        protected:
            void *TranslateToPhysical(uint64_t virtualAddress);
            void InitializePhysicalBitmap();

            void *RamPtrFromRelAddress(size_t pageIndex) const;

        private:
            // FIXME: the MMU needs the cache controller, the cache controller needs access to the data-bus
            //        there should only be ONE databus per CPU independently of number of cores..
            //        MMU -> Cache -> DataBus...
            //        So, the DataBus is shared across CORE's but each CORE has their own MMU/Cache handling
            //        Which makes sense since MMU Tables should be handled as regular memory access (read/write)
            //CacheController cacheController;

            RegisterValue mmuControl = {};
            RegisterValue mmuPageTableAddress = {};

            void *ptrPhysicalRamStart = nullptr;
            size_t szPhysicalRam = 0;

            uint8_t *ptrFirstAvailablePage = nullptr;
            size_t idxCurrentPhysicalPage = 0;

            uint8_t *phyBitMap = nullptr;
            size_t szBitmapBytes = 0;
            // Root tables are in the virtual address space...
            PageTable *rootTables;
        };
    }
}



#endif //MEMORYUNIT_H
