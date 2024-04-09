//
// Created by gnilk on 09.04.24.
//

#include <stdint.h>
#include <vector>
#include <filesystem>
#include <testinterface.h>

#include "System.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
DLL_EXPORT int test_soc(ITesting *t);
DLL_EXPORT int test_soc_defaults(ITesting *t);
}

DLL_EXPORT int test_soc(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_soc_defaults(ITesting *t) {

    SoC::Instance().SetDefaults();

    return kTR_Pass;
}


