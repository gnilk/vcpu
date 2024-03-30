//
// Created by gnilk on 30.03.24.
//
#include <testinterface.h>
#include "MemorySubSys/CPUMemCache.h"

using namespace gnilk;
using namespace gnilk::vcpu;


extern "C" {
DLL_EXPORT int test_cache(ITesting *t);
}
DLL_EXPORT int test_cache(ITesting *t) {
    return kTR_Pass;
}

