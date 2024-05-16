//
// Created by gnilk on 16.05.24.
//

#include <stdint.h>
#include "InstructionDecoderBase.h"
#include "CPUBase.h"

using namespace gnilk;
using namespace gnilk::vcpu;

uint8_t InstructionDecoderBase::NextByte(CPUBase &cpu) {
    // Note: FetchFromRam will modifiy the address!!!!
    auto nextByte = cpu.FetchFromPhysicalRam<uint8_t>(memoryOffset);
    return nextByte;
}
