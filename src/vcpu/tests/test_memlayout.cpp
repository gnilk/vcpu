//
// Created by gnilk on 15.05.24.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>
#include <thread>
#include <chrono>

#include "VirtualCPU.h"
#include "Timer.h"

using namespace gnilk;
using namespace gnilk::vcpu;

static uint8_t ram[32*4096]; // 32 pages more than enough for this test


extern "C" {
DLL_EXPORT int test_memlayout(ITesting *t);
}


static MemoryLayout layout  __attribute__ ((aligned (65536)));
DLL_EXPORT int test_memlayout(ITesting *t) {
    printf("Memory Layout Size=%zu bytes\n", sizeof(MemoryLayout));
    printf("Layout ptr=%p\n", &layout);
    printf("Layout.isrVectorTable ptr=%p, size=%zu (%x)\n", &layout.isrVectorTable, sizeof(layout.isrVectorTable), (int)sizeof(layout.isrVectorTable));
    printf("Layout.ExceptionControlBase ptr=%p, size=%zu (%x)\n", &layout.exceptionControlBlock, sizeof(layout.exceptionControlBlock), (int)sizeof(layout.exceptionControlBlock));
    printf("Memory Page Addr Size=%zu bytes\n", VCPU_MMU_PAGE_SIZE);
    return kTR_Pass;
}
