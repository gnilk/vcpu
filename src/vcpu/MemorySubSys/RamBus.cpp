//
// Created by gnilk on 31.03.2024.
//

#include "RamBus.h"
#include "System.h"
#include <string.h>

using namespace gnilk::vcpu;

RamMemory::RamMemory(size_t szRam) {
    data = new uint8_t[szRam];
    numBytes = szRam;
    isExternallyManaged = false;
    memset(data,0,numBytes);
}

RamMemory::RamMemory(void *ptr, size_t szRam) {
    data = static_cast<uint8_t *>(ptr);
    numBytes = szRam;
    isExternallyManaged = true;
}

RamMemory::~RamMemory() {
    if (!isExternallyManaged) {
        delete[] data;
    }
}


void RamMemory::Write(uint64_t addrDescriptor, const void *src) {
    memcpy(&data[addrDescriptor], src, GNK_L1_CACHE_LINE_SIZE);
}

void RamMemory::Read(void *dst, uint64_t addrDescriptor) {
    memcpy(dst, &data[addrDescriptor], GNK_L1_CACHE_LINE_SIZE);
}

void RamMemory::WriteVolatile(uint64_t addrDescriptor, const void *src, size_t nBytes) {
    memcpy(&data[addrDescriptor], src, nBytes);
}
void RamMemory::ReadVolatile(void *dst, uint64_t addrDescriptor, size_t nBytes) {
    memcpy(dst, &data[addrDescriptor], nBytes);
}


///////////
MesiBusBase::Ref RamBus::Create(size_t szRam) {
    auto instance = std::make_shared<RamBus>();
    instance->SetRamMemory(new RamMemory(szRam));
    return instance;
}
MesiBusBase::Ref RamBus::Create(RamMemory *ptrRam) {
    auto instance = std::make_shared<RamBus>();
    instance->SetRamMemory(ptrRam);
    return instance;
}


void RamBus::ReadLine(void *dst, uint64_t addrDescriptor) {
    // FIXME: Not sure this should be done here...
    ram->Read(dst, addrDescriptor & VCPU_MEM_ADDR_MASK);
}

void RamBus::WriteLine(uint64_t addrDescriptor, const void *src) {
    // FIXME: Not sure this should be done here...
    ram->Write(addrDescriptor & VCPU_MEM_ADDR_MASK, src);
}

