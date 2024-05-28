//
// Created by gnilk on 27.05.24.
//
#include <testinterface.h>

#include "InstructionSetV1/InstructionSetV1.h"
#include "Simd/SIMDInstructionSet.h"
#include "InstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;



extern "C" {
DLL_EXPORT int test_main(ITesting *t);
DLL_EXPORT int test_exit(ITesting *t);
}

//
// Make sure we populate the Instruction manager...
//
DLL_EXPORT int test_main(ITesting *t) {
    // Make sure we register the instruction sets properly.
    if (!InstructionSetManager::Instance().HaveInstructionSet()) {
        InstructionSetManager::Instance().SetInstructionSet<InstructionSetV1>();
    }
    if (!InstructionSetManager::Instance().HaveExtension(OperandCode::SIMD)) {
        InstructionSetManager::Instance().RegisterExtension<InstructionSetSIMD>(OperandCode::SIMD);
    }

    return kTR_Pass;
}
DLL_EXPORT int test_exit(ITesting *t) {
    return kTR_Pass;
}


