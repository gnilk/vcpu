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

RamBus &RamBus::Instance() {
    static RamBus glbBus;
    return glbBus;
}

void MesiBusBase::Subscribe(uint8_t idCore, MessageHandler cbOnMessage) {
    if (idCore >= GNK_CPU_NUM_CORES) {
        return;
    }
    subscribers[idCore].idCore = idCore;
    subscribers[idCore].cbOnMessage = std::move(cbOnMessage);
    nextSubscriber++;
}

kMESIState MesiBusBase::BroadCastRead(uint8_t idCore, uint64_t addrDescriptor) {
    return SendMessage(kMemOp::kBusRd, idCore, addrDescriptor);
}
void MesiBusBase::BroadCastWrite(uint8_t idCore, uint64_t addrDescriptor) {
    SendMessage(kMemOp::kBusWr, idCore, addrDescriptor);
}

kMESIState MesiBusBase::SendMessage(kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
    // We just want the top bits - the rest is the same regardless, we drag in a full line..
    // Ergo - it makes sense to align array's to CACHE_LINE_SIZE...

    for(auto &snooper : subscribers) {
        if (snooper.idCore == sender) {
            continue;
        }
        if (snooper.cbOnMessage == nullptr) {
            continue;
        }
        auto res = snooper.cbOnMessage(op, sender, addrDescriptor);
        // FIXME: Need to verify this a bit more
        if (res != kMesi_Invalid) {
            return res;
        }
    }
    // No one had this memory block!
    //subscribers[sender].cbOnMessage()


    return kMESIState::kMesi_Invalid;
}

void RamBus::ReadLine(void *dst, uint64_t addrDescriptor) {
    ram->Read(dst, addrDescriptor);
}

void RamBus::WriteLine(uint64_t addrDescriptor, const void *src) {
    ram->Write(addrDescriptor, src);
}

