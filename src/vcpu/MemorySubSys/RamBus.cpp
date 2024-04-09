//
// Created by gnilk on 31.03.2024.
//

#include "RamBus.h"
#include <string.h>

using namespace gnilk::vcpu;

RamMemory::RamMemory(size_t szRam) {
    data = new uint8_t[szRam];
    numBytes = szRam;
    memset(data,0,numBytes);
}

RamMemory::~RamMemory() {
    delete[] data;
}

void RamMemory::Write(uint64_t addrDescriptor, const void *src) {
    memcpy(&data[addrDescriptor], src, GNK_L1_CACHE_LINE_SIZE);
}

void RamMemory::Read(void *dst, uint64_t addrDescriptor) {
    memcpy(dst, &data[addrDescriptor], GNK_L1_CACHE_LINE_SIZE);
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
    ram->Read(dst, addrDescriptor);
}

void RamBus::WriteLine(uint64_t addrDescriptor, const void *src) {
    ram->Write(addrDescriptor, src);
}

