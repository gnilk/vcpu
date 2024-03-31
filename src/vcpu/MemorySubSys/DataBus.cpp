//
// Created by gnilk on 31.03.2024.
//

#include "DataBus.h"

using namespace gnilk::vcpu;

RamMemory::RamMemory(size_t szRam) {
    data = new uint8_t[szRam];
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

DataBus &DataBus::Instance() {
    static DataBus glbBus;
    return glbBus;
}

void DataBus::Subscribe(uint8_t idCore, MessageHandler cbOnMessage) {
    if (idCore >= GNK_CPU_NUM_CORES) {
        return;
    }
    subscribers[idCore].idCore = idCore;
    subscribers[idCore].cbOnMessage = std::move(cbOnMessage);
    nextSubscriber++;
}

kMESIState DataBus::BroadCastRead(uint8_t idCore, uint64_t addrDescriptor) {
    return SendMessage(kMemOp::kBusRd, idCore, addrDescriptor);
}
void DataBus::BroadCastWrite(uint8_t idCore, uint64_t addrDescriptor) {
    SendMessage(kMemOp::kBusWr, idCore, addrDescriptor);
}

kMESIState DataBus::SendMessage(kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
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

void DataBus::ReadMemory(void *dst, uint64_t addrDescriptor) {
    ram->Read(dst, addrDescriptor);
}

void DataBus::WriteMemory(uint64_t addrDescriptor, const void *src) {
    ram->Write(addrDescriptor, src);
}

