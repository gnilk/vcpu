//
// Created by gnilk on 09.04.24.
//

#include "System.h"
#include "FlashBus.h"

using namespace gnilk;
using namespace gnilk::vcpu;

BusBase::Ref FlashBus::Create(RamMemory *ptrRam) {
    auto instance = std::make_shared<FlashBus>();
    instance->SetRamMemory(ptrRam);
    return instance;

}
BusBase::Ref FlashBus::Create(size_t szRam) {
    auto instance = std::make_shared<FlashBus>();
    instance->SetRamMemory(new RamMemory(szRam));
    return instance;
}

void FlashBus::ReadData(void *dst, uint64_t addrDescriptor, size_t nBytes) {
    // FIXME: Need better read/write routines in RAM
    ram->ReadVolatile(dst, addrDescriptor & VCPU_MEM_ADDR_MASK, nBytes);
}

void FlashBus::WriteData(uint64_t addrDescriptor, const void *src, size_t nBytes) {
    // FIXME: Need better read/write routines in RAM
    ram->WriteVolatile(addrDescriptor & VCPU_MEM_ADDR_MASK, src, nBytes);
}

