//
// Created by gnilk on 02.04.2024.
//

#ifndef VCPU_PAGEALLOCATOR_H
#define VCPU_PAGEALLOCATOR_H

#include "MemoryUnit.h"

namespace gnilk {
    namespace vcpu {
        class PageAllocator {
        public:
            PageAllocator() = default;
            virtual ~PageAllocator() = default;
            uint64_t AllocatePage(const MMU &mmu);
            bool FreePage(const MMU &mmu, uint64_t virtualAddress);
        protected:
            void InitializePageTables(const MMU &mmu);
            int64_t FindFreeRootTable(const MMU &mmu);
            int64_t FindFreePageDescriptor(const MMU &mmu, uint64_t idxRootTable);
            int64_t FindFreePage(const MMU &mmu, uint64_t idxRootTable, uint64_t idxDescriptor);
        };
    }
}


#endif //VCPU_PAGEALLOCATOR_H
