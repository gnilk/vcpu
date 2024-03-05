//
// Created by gnilk on 08.01.2024.
//

#ifndef MEMORYUNIT_H
#define MEMORYUNIT_H

#include <stdlib.h>
#include <stdint.h>
#include <unordered_map>

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

        // Placeholders to have Control Registers some what marked up
        struct MMUControl0 {
            uint64_t empty;
        };
        struct MMUControl1 {
            uint64_t empty;
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
#pragma pack(push,1)
            // 32 bytes
            struct Page {
                void *ptrPhysical = nullptr;    // 64 bit?
                uint32_t bytes = 0;             // 32 bit, page size - max 4gb continous memory
                uint32_t flags = 0;  // TBD     // 32 bit
            };
            struct PageDescriptor {
                size_t nPages = 0;
                Page pages[VCPU_MMU_MAX_PAGES_PER_DESC];
            };
            struct PageTable {
                size_t nDescriptors = 0;
                PageDescriptor descriptor[VCPU_MMU_MAX_DESCRIPTORS];
            };
#pragma pack(pop)

        public:
            MemoryUnit() = default;
            virtual ~MemoryUnit() = default;
            bool Initialize(void *physicalRam, size_t sizeInBytes);
            bool IsAddressValid(uint64_t virtualAddress);

            uint64_t TranslateAddress(uint64_t virtualAddress);
            uint64_t AllocatePage();
            bool FreePage(uint64_t virtualAddress);
            // debugging / unit-test functionality
            const Page *GetPageForAddress(uint64_t virtualAddress);
            const uint8_t *GetPhyBitmapTable() {
                return phyBitMap;
            }
            size_t GetPhyBitmapTableSize() {
                return szBitmapBytes;
            }
        protected:
            void *AllocatePhysicalPage(uint64_t virtualAddress);
            void FreePhysicalPage(uint64_t virtualAddress);
            void InitializeRootTables();

            int64_t FindFreeRootTable();
            int64_t FindFreePageDescriptor(uint64_t idxRootTable);
            int64_t FindFreePage(uint64_t idxRootTable, uint64_t idxDescriptor);

            inline uint64_t IdxRootTableFromVA(uint64_t virtualAddress) {
                return (virtualAddress >> (12 + VCPU_MMU_DESCRIPTORS_NBITS + VCPU_MMU_PAGES_NBITS)) & (VCPU_MMU_MAX_ROOT_TABLES-1);
            }
            inline uint64_t IdxDescriptorFromVA(uint64_t virtualAddress) {
                return (virtualAddress >> (12 + VCPU_MMU_PAGES_NBITS)) & (VCPU_MMU_MAX_DESCRIPTORS - 1);
            }
            inline uint64_t IdxPageFromVA(uint64_t virtualAddress) {
                return (virtualAddress >> 12) & (VCPU_MMU_MAX_PAGES_PER_DESC-1);
            }
        private:
            void *ptrPhysicalRamStart = nullptr;
            size_t szPhysicalRam = 0;

            uint8_t *ptrCurrentPage = nullptr;
            size_t idxCurrentPhysicalPage = 0;

            uint8_t *phyBitMap = nullptr;
            size_t szBitmapBytes = 0;
            PageTable *rootTables;
        };
    }
}



#endif //MEMORYUNIT_H
