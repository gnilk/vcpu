//
// Created by gnilk on 16.05.24.
//

#include "SIMDInstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;

std::unordered_map<SimdOpCode, SimdOperandDescription> SIMDInstructionSet::instructionSet = {
        {SimdOpCode::ABS,
            {
                 .name = "abs",
                 .features = SimdOpCodeFeatureFlags::kFeature_Float \
                             | SimdOpCodeFeatureFlags::kFeature_Integer \
                             | SimdOpCodeFeatureFlags::kFeature_Condition \
                             | SimdOpCodeFeatureFlags::kFeature_Clamping \
                             | SimdOpCodeFeatureFlags::kFeature_HalfPrecision \
                             | SimdOpCodeFeatureFlags::kFeature_Def_Float,
            },
        },
        {SimdOpCode::ADD,
                {
                        .name = "add",
                        .features = SimdOpCodeFeatureFlags::kFeature_Float \
                             | SimdOpCodeFeatureFlags::kFeature_Integer \
                             | SimdOpCodeFeatureFlags::kFeature_Condition \
                             | SimdOpCodeFeatureFlags::kFeature_Clamping \
                             | SimdOpCodeFeatureFlags::kFeature_HalfPrecision \
                             | SimdOpCodeFeatureFlags::kFeature_Def_Float,
                },
        },
        {SimdOpCode::AND,
                {
                        .name = "and",
                        .features = SimdOpCodeFeatureFlags::kFeature_Integer \
                             | SimdOpCodeFeatureFlags::kFeature_Condition \
                             | SimdOpCodeFeatureFlags::kFeature_Def_Signed,
                },
        },
        {SimdOpCode::BRK,{.name="brk", .features = {} }},
        {SimdOpCode::CALL,{.name="call", .features = {} }},
};

std::unordered_map<SimdOpCode, SimdOperandDescription> &SIMDInstructionSet::GetInstructionSet() {
    return instructionSet;
}
