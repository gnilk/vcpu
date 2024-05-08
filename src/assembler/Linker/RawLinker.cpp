//
// Created by gnilk on 08.05.24.
//

#include "RawLinker.h"

using namespace gnilk;
using namespace gnilk::assembler;

static void VectorReplaceAt(std::vector<uint8_t> &data, uint64_t offset, int64_t newValue, vcpu::OperandSize opSize);

// Should this be const????
bool RawLinker::Link(Context &srcContext) {
    // Just merge the source context..
    if (!srcContext.Merge()) {
        return false;
    }

    // This should be enough..
    linkedData.clear();
    linkedData.insert(linkedData.begin(), srcContext.Data().begin(), srcContext.Data().end());
    return true;
}

const std::vector<uint8_t> &RawLinker::Data() {
    return linkedData;
}
