//
// Created by gnilk on 28.05.24.
//
#include <stdint.h>
#include "Dispatch.h"
#include <testinterface.h>

using namespace gnilk;
using namespace gnilk::vcpu;

static uint8_t ram[32*4096]; // 32 pages more than enough for this test

extern "C" {
DLL_EXPORT int test_dispatch(ITesting *t);
DLL_EXPORT int test_dispatch_fill(ITesting *t);
}
DLL_EXPORT int test_dispatch(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_dispatch_fill(ITesting *t) {
    return kTR_Pass;
}
