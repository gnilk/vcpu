//
// Created by gnilk on 10.04.24.
//

#include <memory>
#include "System.h"
#include "HWMappedBus.h"
using namespace gnilk;
using namespace gnilk::vcpu;

BusBase::Ref HWMappedBus::Create() {
    auto instance = std::make_shared<HWMappedBus>();
    return instance;
}

void HWMappedBus::ReadData(void *dst, uint64_t addrDescriptor, size_t nBytes) {
    if (onRead != nullptr) {
        onRead(dst, addrDescriptor & VCPU_SOC_ADDR_MASK, nBytes);
    }
}
void HWMappedBus::WriteData(uint64_t addrDescriptor, const void *src, size_t nBytes) {
    if (onWrite != nullptr) {
        onWrite(addrDescriptor & VCPU_SOC_ADDR_MASK, src, nBytes);
    }

}