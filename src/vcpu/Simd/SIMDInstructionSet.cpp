//
// Created by gnilk on 16.05.24.
//

#include "SIMDInstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;

std::unordered_map<SimdOpCode, SimdOperandDescription> SIMDInstructionSet::instructionSet = {
        {SimdOpCode::LOAD,
            {
                 .name = "ve_load",
                 .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_AddressReg \
                             | SimdOpCodeFeatureFlags::kFeature_Mask \
                             | SimdOpCodeFeatureFlags::kFeature_Advance,
            },
        },
        {SimdOpCode::STORE,
                {
                        .name = "ve_store",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_AddressReg \
                             | SimdOpCodeFeatureFlags::kFeature_Mask \
                             | SimdOpCodeFeatureFlags::kFeature_Advance,
                },
        },
        {SimdOpCode::VMUL,
                {
                        .name = "ve_mul",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_Mask,
                },
        },
        {SimdOpCode::HADD,
                {
                        .name = "ve_hadd",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_Mask,
                },
        },
        {SimdOpCode::UNPCKFP8,
                {
                        .name = "ve_unpckfp8",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_Mask,
                },
        },
        {SimdOpCode::UNPCKFP16,
                {
                        .name = "ve_unpckfp16",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_Mask,
                },
        },
        {SimdOpCode::PACKFP16,
                {
                        .name = "ve_packfp16",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_Mask,
                },
        },
        {SimdOpCode::PACKFP32,
                {
                        .name = "ve_packfp32",
                        .features = SimdOpCodeFeatureFlags::kFeature_Size \
                             | SimdOpCodeFeatureFlags::kFeature_Mask,
                },
        },

};

std::unordered_map<SimdOpCode, SimdOperandDescription> &SIMDInstructionSet::GetInstructionSet() {
    return instructionSet;
}
