//
// Created by gnilk on 08.01.2024.
//

#ifndef MEMORYUNIT_H
#define MEMORYUNIT_H

#include <stdlib.h>
#include <stdint.h>
#include <unordered_map>

#include "CacheController.h"
#include "RegisterValue.h"
#include "MemoryRegion.h"

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
            kMMU_TranslationEnabled = 1,        // if we should look up address in the page-translation table..
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
        struct PageTableDescriptor {
            size_t nPages = 0;  // need to remove this - must have better structure!
            Page pages[VCPU_MMU_MAX_PAGES_PER_DESC];
        };
        // VCPU_MMU_MAX_DESCRIPTORS * 16 * VCPU_MMU_MAX_PAGES_PER_DESC + 8 + 8
        struct PageTableEntry {
            size_t nDescriptors = 0;    // need to remove this - must have better structure
            PageTableDescriptor descriptor[VCPU_MMU_MAX_DESCRIPTORS];
        };

#pragma pack(pop)

//        Upper 32                            |              Lower 32
//        bit   63                                                                              0
//        xxxx RRRR 0000 0000 0000 0000 0000 TTTT | TTTT DDDD DDDD PPPP PPPP AAAA AAAA AAAA
//
//        This gives 36 bit linear address space per mapped region!
//        Easily extended to 42 bits (just more levels of tables).
//
//
//       Not quite sure how I should handle these...
//
//        RRRR - Region, the region is a 4 bit lookup inside the MMU - regions are predefined and can not change in runtime
//               The MMU supports MAX 16 regions of memory...
//        xxxx - Reserved, should be left to zero...
//
//  Ex:
//        TTTT - Table index
//        DDDD - Descriptor index
//        PPPP - Page Table Entry index
//        X - Virtual address space start marker (will this impose a stupid limit somewhere?)
//        Any address below is reserved..
//        Which means if you do 'void *ptr = 0x1234'    <- 'marker' not set, this will be an illegal address and considered for OS use..
//        AAAA - 12 bit page offset
//
//                A region defines:
//        Valid Address Range; up to 56 bits [start/stop 64bit addresses - must be maskable]
//        FLAGS
//                Cache-able -> should be pulled in/out of the L1 Cache or is to be treated as pass-through
//                R/W/X -> Read/Write/Execute
//
//                DMA -> can be addressed through the DMA
//        Access Exception Handler
//                What to call in case of access; void AccessExceptionHandler(Operation, Address)
//        When the AccessExceptionHandler is hit, the CPU will read/write a full page of data and copy that to RAM.
//
//        For EMU
//        some kind of subsystem indicator
//                ptr to memory area
//
//                Region '0' is used for FastRAM (SRAM) addresses               8                     8
//        Valid Address Range; 0..X (where X is MAX_RAM-1), like; 0x00800000 00000000 - 0x00800000 0000ffff (if we give it 64k of RAM)
//        This is managed by the emulated EMU when 'SetRam' is called on the MMU
//
//
//                Region '1' is HW mapped registers and busses (4gb)
//        Valid Address Range: 0x01000000 00000000 - 0x01000000 ffffffff      (this is the maximum range)
//        This is managed by the CPU which defines this table and gives it to the MMU
//
//        Region '2' is Ext. RAM
//                Valid Address Range: 0x02000000 00000000 - 0x01000000 ffffffff
//
//                Region '3' is Flash or non-volatile storage
//                Valid Address Range: 0x03000000 00000000 - 0x01000000 ffffffff
//


        static const uint64_t VCPU_MMU_PAGE_OFFSET_MASK = 0x0000'0000'0000'0fff;

        static const uint64_t VCPU_MMU_PAGE_ENTRY_IDX_MASK = 0x0000'0000'000f'f000;
        static const uint64_t VCPU_MMU_PAGE_ENTRY_SHIFT = (12);

        static const uint64_t VCPU_MMU_PAGE_DESC_IDX_MASK = 0x0000'0000'0ff0'0000;
        static const uint64_t VCPU_MMU_PAGE_DESC_SHIFT = (12+8);

        static const uint64_t VCPU_MMU_PAGE_TABLE_IDX_MASK = 0x0000'000f'f000'0000;
        static const uint64_t VCPU_MMU_PAGE_TABLE_SHIFT = (12+16);


        // New version
        class MMU {
        public:
            MMU() = default;
            virtual ~MMU() = default;

            void Initialize(uint8_t newCoreId);

            bool IsAddressValid(uint64_t address);


            __inline uint8_t constexpr RegionFromAddress(uint64_t virtualAddress) {
                uint8_t region = (virtualAddress & VCPU_MEM_REGION_MASK) >> (VCPU_MEM_REGION_SHIFT);
                return region;
            }

            __inline uint64_t constexpr RegionFromIndex(uint64_t regionIndex) {
                uint8_t region = (regionIndex & 0x0f) << (VCPU_MEM_REGION_SHIFT);
                return region;
            }

            __inline uint64_t constexpr PageTableIndexFromAddress(uint64_t virtualAddress) {
                uint64_t offset = (virtualAddress & VCPU_MMU_PAGE_TABLE_IDX_MASK) >> (VCPU_MMU_PAGE_TABLE_SHIFT);
                return offset;
            }
            __inline uint64_t constexpr PageDescriptorIndexFromAddress(uint64_t virtualAddress) {
                uint64_t offset = (virtualAddress & VCPU_MMU_PAGE_DESC_IDX_MASK) >> (VCPU_MMU_PAGE_DESC_SHIFT);
                return offset;
            }
            __inline uint64_t constexpr PageTableEntryIndexFromAddress(uint64_t virtualAddress) {
                uint64_t offset = (virtualAddress & VCPU_MMU_PAGE_ENTRY_IDX_MASK) >> (VCPU_MMU_PAGE_ENTRY_SHIFT);
                return offset;
            }
            __inline uint64_t constexpr PageOffsetFromAddress(uint64_t virtualAddress) {
                uint64_t offset = (virtualAddress & VCPU_MMU_PAGE_OFFSET_MASK);
                return offset;
            }


            void SetMMUControl(const RegisterValue &newControl);
            void SetMMUControl(RegisterValue &&newControl);
            void SetMMUPageTableAddress(const RegisterValue &newPageTblAddr);


            template<typename T>
            int32_t Write(uint64_t virtualAddress, const T &value) {
                static_assert(std::is_integral_v<T> == true);

                uint8_t data[sizeof(T)];
                size_t index = 0;
                auto numToWrite = sizeof(T);
                auto bitShift = (numToWrite-1)<<3;
                while(numToWrite > 0) {

                    data[index] = (value >> bitShift) & 0xff;

                    bitShift -= 8;
                    index++;
                    numToWrite -= 1;
                }

                return WriteInternalFromExternal(virtualAddress, data, sizeof(T));
            }

            template<typename T>
            T Read(uint64_t virtualAddress) {
                static_assert(std::is_integral_v<T> == true);
                uint8_t data[sizeof(T)];
                //T value;
                ReadInternalToExternal(data, virtualAddress, sizeof(T));

                // Not sure if this actually should be here!

                T result = {};

                size_t index = 0;
                uint32_t nBits = 0;
                auto numToFetch = sizeof(T);
                while(numToFetch > 0) {

                    // FIXME: delegate to 'memory-reader'
                    auto byte = data[index];
                    index++;
                    // this results in 'hex' dumps easier to read - MSB first (most significant byte first) - Intel?
                    result = (result << nBits) | T(byte);
                    nBits = 8;

                    // this results in LSB - least significant byte first (Motorola?)
                    // result |= T(byte) << nBits);
                    // nBits += 8;
                    numToFetch -= 1;
                }


                return result;
            }

            void Touch(const uint64_t address);


            // These two functions will bypass the cache!!!
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
            int32_t WriteInternalFromExternal(uint64_t address, const void *src, size_t nBytes);
            void ReadInternalToExternal(void *dst, uint64_t address, size_t nBytes);

        protected:
            uint8_t coreId = 0;
            RegisterValue mmuControl;
            RegisterValue mmuPageTableAddress;
            // This cache controller has ability to cache any kind of memory access...
            CacheController cacheController;
        };



        //
        // This is the old MMU
        //
/*
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


            PageTableEntry *GetPageTables() const;

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
            PageTableEntry *rootTables;
        };
*/
    }
}



#endif //MEMORYUNIT_H
