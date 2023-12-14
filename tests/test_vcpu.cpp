//
// Created by gnilk on 14.12.23.
//
#include <testinterface.h>

extern "C" {
    DLL_EXPORT int test_vcpu(ITesting *t);
    DLL_EXPORT int test_vcpu_create(ITesting *t);
}

DLL_EXPORT int test_vcpu(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_vcpu_create(ITesting *t) {
    return kTR_Pass;
}
