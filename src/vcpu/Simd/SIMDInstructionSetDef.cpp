//
// Created by gnilk on 16.05.24.
//

#include "SIMDInstructionSetDef.h"

using namespace gnilk;
using namespace gnilk::vcpu;

std::unordered_map<OperandCodeBase , OperandDescriptionBase> SIMDInstructionSetDef::instructionSet = {
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

const std::unordered_map<OperandCodeBase, OperandDescriptionBase> &SIMDInstructionSetDef::GetInstructionSet() {
    return instructionSet;
}
std::optional<OperandDescriptionBase> SIMDInstructionSetDef::GetOpDescFromClass(OperandCodeBase opClass) {
    return {};
}
std::optional<OperandCodeBase> SIMDInstructionSetDef::GetOperandFromStr(const std::string &str) {
    return {};
}

