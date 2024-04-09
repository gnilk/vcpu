//
// Created by gnilk on 30.03.24.
//
#include <string.h>
#include <testinterface.h>
#include "MemorySubSys/CPUMemCache.h"

using namespace gnilk;
using namespace gnilk::vcpu;


extern "C" {
DLL_EXPORT int test_cache(ITesting *t);
DLL_EXPORT int test_cache_touch(ITesting *t);
DLL_EXPORT int test_cache_read(ITesting *t);
DLL_EXPORT int test_cache_write(ITesting *t);
DLL_EXPORT int test_cache_sync(ITesting *t);
}

#define RAM_SIZE 65536

DLL_EXPORT int test_cache(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_cache_touch(ITesting *t) {
    int32_t value = 4711;
    uint8_t buffer[2048];
//    RamMemory ram(65536);
//    //DataBus bus(ram);
//    RamBus::Instance().SetRamMemory(&ram);
    auto rambus = RamBus::Create(RAM_SIZE);

    CacheController cacheControllerA;
    CacheController cacheControllerB;

    cacheControllerA.Initialize(0, rambus);
    cacheControllerB.Initialize(1, rambus);

    // This should give us ONE line in the cache.
    cacheControllerA.Touch(0x4711);

    auto &cacheA = cacheControllerA.GetCache();
    auto &cacheB = cacheControllerB.GetCache();

    std::unordered_map<kMESIState, int> cache_a_states;
    std::unordered_map<kMESIState, int> cache_b_states;

    for(int i=0;i<cacheA.GetNumLines();i++) {
        cache_a_states[cacheA.GetLineState(i)] += 1;
        cache_b_states[cacheB.GetLineState(i)] += 1;

    }
    TR_ASSERT(t, cache_a_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_a_states[kMesi_Exclusive] == 1);
    // All 'b' should be invalid
    TR_ASSERT(t, cache_b_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES);


    return kTR_Pass;
}

DLL_EXPORT int test_cache_read(ITesting *t) {
    uint8_t buffer[2048];
    RamMemory ram(RAM_SIZE);
    auto rambus = RamBus::Create(&ram);

    CacheController cacheControllerA;
    CacheController cacheControllerB;

    cacheControllerA.Initialize(0,rambus);
    cacheControllerB.Initialize(1,rambus);

    // Plan a value in the emulated RAM - bypassing the cache
    int plantedValue = 4711;
    void *ptrValue = ram.PtrToRamAddress(0x4711);
    memcpy(ptrValue, &plantedValue, sizeof(int));

    // Now read the address - this should pull in a full cache line in exclusive mode
    auto value = cacheControllerA.Read<int>(0x4711);

    auto &cacheA = cacheControllerA.GetCache();
    auto &cacheB = cacheControllerB.GetCache();

    std::unordered_map<kMESIState, int> cache_a_states;
    std::unordered_map<kMESIState, int> cache_b_states;

    for(int i=0;i<cacheA.GetNumLines();i++) {
        cache_a_states[cacheA.GetLineState(i)] += 1;
        cache_b_states[cacheB.GetLineState(i)] += 1;

    }
    TR_ASSERT(t, cache_a_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_a_states[kMesi_Exclusive] == 1);
    // All 'b' should be invalid
    TR_ASSERT(t, cache_b_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES);


    return kTR_Pass;

}

DLL_EXPORT int test_cache_write(ITesting *t) {
    int32_t value = 4711;
    uint8_t buffer[2048];
    RamMemory ram(RAM_SIZE);
    auto rambus = RamBus::Create(&ram);

    CacheController cacheControllerA;
    CacheController cacheControllerB;

    cacheControllerA.Initialize(0, rambus);
    cacheControllerB.Initialize(1, rambus);

    // This should give us ONE line in the cache.
    cacheControllerA.Write<int>(0x4711, 0x1234);

    auto &cacheA = cacheControllerA.GetCache();
    auto &cacheB = cacheControllerB.GetCache();

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


    return kTR_Pass;

}
DLL_EXPORT int test_cache_sync(ITesting *t) {
    RamMemory ram(RAM_SIZE);
    auto rambus = RamBus::Create(&ram);

    CacheController cacheControllerA;
    CacheController cacheControllerB;

    cacheControllerA.Initialize(0, rambus);
    cacheControllerB.Initialize(1, rambus);

    // Issue a write on Core 0
    cacheControllerA.Write<int>(0x4711, 0x1234);

    auto &cacheA = cacheControllerA.GetCache();
    auto &cacheB = cacheControllerB.GetCache();

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

    // Issue a Read on Core 1
    auto value = cacheControllerB.Read<int>(0x4711);
    TR_ASSERT(t, value == 0x1234);

    cache_a_states = {};
    cache_b_states = {};

    for(int i=0;i<cacheA.GetNumLines();i++) {
        cache_a_states[cacheA.GetLineState(i)] += 1;
        cache_b_states[cacheB.GetLineState(i)] += 1;
    }
    // We should have shared cache line in each...
    TR_ASSERT(t, cache_a_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_a_states[kMesi_Shared] == 1);

    TR_ASSERT(t, cache_b_states[kMesi_Invalid] == GNK_L1_CACHE_NUM_LINES-1);
    TR_ASSERT(t, cache_a_states[kMesi_Shared] == 1);

    cacheControllerA.Flush();
    cache_a_states = {};
    cache_b_states = {};

    for(int i=0;i<cacheA.GetNumLines();i++) {
        cache_a_states[cacheA.GetLineState(i)] += 1;
        cache_b_states[cacheB.GetLineState(i)] += 1;
    }


    return kTR_Pass;
}

