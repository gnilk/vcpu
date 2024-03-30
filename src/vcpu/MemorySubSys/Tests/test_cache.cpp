//
// Created by gnilk on 30.03.24.
//
#include <testinterface.h>
#include "MemorySubSys/CPUMemCache.h"

using namespace gnilk;
using namespace gnilk::vcpu;


extern "C" {
DLL_EXPORT int test_cache(ITesting *t);
DLL_EXPORT int test_cache_sync(ITesting *t);
}
DLL_EXPORT int test_cache(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_cache_sync(ITesting *t) {
    int32_t value = 4711;
    uint8_t buffer[2048];
    RamMemory ram(65536);
    DataBus bus(ram);

    CacheController cacheControllerA(bus, 0);
    CacheController cacheControllerB(bus, 1);
    CacheController cacheControllerC(bus, 2);
    CacheController cacheControllerD(bus, 3);

    //
    // on CoreA (CacheControllerA) we 'write' 4711 (to virtual address 0x4711)
    // What will happen is that CORE A brings the virtual memory for 0x4711 into the cache and writes the value 4711
    // to it's cache line..  Leaving it there..
    //

    auto &cacheA = cacheControllerA.GetCache();
    auto &cacheB = cacheControllerB.GetCache();
    for(int i=0;i<cacheA.GetNumLines();i++) {
        TR_ASSERT(t, cacheA.GetLineState(i) == kMesi_Invalid);
        TR_ASSERT(t, cacheB.GetLineState(i) == kMesi_Invalid);

    }
    void *ptrRamDst = ram.TranslateAddress((void *)0x4711);
    auto nWritten = cacheControllerA.Write(ptrRamDst, &value, sizeof(value));
    TR_ASSERT(t, nWritten == 4);

    // Here we will only see one line on CoreA that has state 'modified'
    cacheControllerA.Dump();
    cacheControllerB.Dump();

    std::unordered_map<kMESIState, int> cache_a_states;
    std::unordered_map<kMESIState, int> cache_b_states;

    for(int i=0;i<cacheA.GetNumLines();i++) {
        cache_a_states[cacheA.GetLineState(i)] += 1;
        cache_b_states[cacheB.GetLineState(i)] += 1;

    }
    TR_ASSERT(t, cache_a_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_a_states[kMesi_Modified] == 1);
    // All 'b' should be invalid
    TR_ASSERT(t, cache_b_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES);


    //
    // on CoreB we read from address 0x4711. This data resides in the cache for CoreA. This will cause a flush of CoreA
    // and then CoreB will be able to fetch the value and change the line to shared...
    //
    int32_t outValue = 0;
    auto nRead = cacheControllerB.Read(&outValue, ptrRamDst, sizeof(value));
    TR_ASSERT(t, nRead == 4);
    TR_ASSERT(t, outValue == 4711);

    cache_a_states = {};
    cache_b_states = {};
    for(int i=0;i<cacheA.GetNumLines();i++) {
        cache_a_states[cacheA.GetLineState(i)] += 1;
        cache_b_states[cacheB.GetLineState(i)] += 1;
    }

    // a should have 1 shared, the modified should now be gone..
    TR_ASSERT(t, cache_a_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_a_states[kMesi_Shared] == 1);
    TR_ASSERT(t, cache_a_states.find(kMesi_Modified) == cache_a_states.end());

    // b should have 1 shared, rest invalid
    TR_ASSERT(t, cache_b_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_b_states[kMesi_Shared] == 1);

    cacheControllerA.Dump();
    cacheControllerB.Dump();

    return kTR_Pass;
}

