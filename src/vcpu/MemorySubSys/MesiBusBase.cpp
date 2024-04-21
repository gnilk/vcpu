//
// Created by gnilk on 09.04.24.
//

#include "MesiBusBase.h"

using namespace gnilk;
using namespace gnilk::vcpu;

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
