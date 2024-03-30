#include <iostream>
#include <array>
#include <functional>
#include <string.h>
#include <unordered_map>

#include "CPUMemCache.h"

//
// Very small and simple MESI SMP cache coherency implementation
// see: https://en.wikipedia.org/wiki/MESI_protocol
//

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
    ram.Read(dst, addrDescriptor);
}

void DataBus::WriteMemory(uint64_t addrDescriptor, const void *src) {
    ram.Write(addrDescriptor, src);
}

//////////////////////////////////////////////////////////////////////////
//
// Cache
//
int Cache::GetLineIndex(uint64_t addrDescriptor) {
    // Replace with binary search
    for(size_t i=0;i<lines.size();i++) {
        if ((lines[i].addrDescriptor == addrDescriptor) && (lines[i].state != kMesi_Invalid)) {
            return (int)i;
        }
    }
    return -1;
}

int Cache::NextLineIndex() {
    int next = 0;
    for(size_t i=1;i<lines.size();i++) {
        if (lines[i].time <= lines[next].time) {
            next = (int)i;
        }
    }
    return next;
}

kMESIState Cache::GetLineState(int idxLine) {
    return lines[idxLine].state;
}

kMESIState Cache::SetLineState(int idxLine, kMESIState newState) {
    lines[idxLine].state = newState;
    return newState;
}

void Cache::ResetLine(int idxLine) {
    lines[idxLine].state = kMesi_Invalid;
    lines[idxLine].time = 0;
}

uint64_t Cache::GetLineAddrDescriptor(int idxLine) {
    return lines[idxLine].addrDescriptor;
}

void Cache::ReadLineData(void *dst, int idxLine) {
    memcpy(dst, lines[idxLine].data, GNK_L1_CACHE_LINE_SIZE);
}

void Cache::WriteLineData(int idxLine, const void *src, uint64_t addrDescriptor, kMESIState state) {
    auto &line = lines[idxLine];
    memcpy(line.data, src, GNK_L1_CACHE_LINE_SIZE);
    line.time++;
    line.state = state;
    line.addrDescriptor = addrDescriptor;
}

// This copies only data - without any updates or anything...
size_t Cache::CopyFromLine(void *dst, int idxLine, uint16_t offset, size_t nBytes) {

    if ((offset + nBytes) > GNK_L1_CACHE_LINE_SIZE) {
        nBytes = GNK_L1_CACHE_LINE_SIZE - offset;
    }
    if (!nBytes) {
        return 0;
    }

    memcpy(dst, &lines[idxLine].data[offset], nBytes);
    return nBytes;
}

size_t Cache::CopyToLine(int idxLine, uint16_t offset, const void *src, size_t nBytes) {
    if ((offset + nBytes) > GNK_L1_CACHE_LINE_SIZE) {
        nBytes = GNK_L1_CACHE_LINE_SIZE - offset;
    }
    if (!nBytes) {
        return 0;
    }
    memcpy(&lines[idxLine].data[offset], src, nBytes);
    lines[idxLine].state = kMesi_Modified;
    return nBytes;
}

void Cache::DumpCacheLines() {
    for(int i=0;i<lines.size();i++) {
        printf("  %d  state=%s, time=%d, desc=0x%x\n",i, MESIStateToString(lines[i].state).c_str(), lines[i].time, (int)lines[i].addrDescriptor);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Cache Controller
//

CacheController::CacheController(DataBus &dataBus, uint8_t coreIdentifier) :
    bus(dataBus),
    idCore(coreIdentifier) {

    bus.Subscribe(idCore, [this](DataBus::kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
        return OnDataBusMessage(op, sender, addrDescriptor);
    });
}

kMESIState CacheController::OnDataBusMessage(DataBus::kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
    printf("CacheController::OnDataBusMessage");

    switch(op) {
        case DataBus::kMemOp::kBusRd :
            return OnMsgBusRd(addrDescriptor);
        case DataBus::kMemOp::kBusWr :
            OnMsgBusWr(addrDescriptor);
            return kMesi_Invalid;
        default:
            printf("Unknown data bus operation!");
            return kMesi_Invalid;
    }
}

kMESIState CacheController::OnMsgBusRd(uint64_t addrDescriptor) {
    auto idxLine = cache.GetLineIndex(addrDescriptor);

    if (idxLine < 0) {
        return kMesi_Invalid;
    }
    if (cache.GetLineState(idxLine) == kMesi_Modified) {
        WriteMemory(idxLine);
    }
    return cache.SetLineState(idxLine, kMesi_Shared);
}

void CacheController::OnMsgBusWr(uint64_t addrDescriptor) {
    auto idxLine = cache.GetLineIndex(addrDescriptor);

    if (idxLine < 0) {
        return;
    }

    if (cache.GetLineState(idxLine) == kMesi_Modified) {
        WriteMemory(idxLine);
        cache.ResetLine(idxLine);
    }
}

//
// Read from cache - 'src' is a cache-line address, dst is somewhere the user wants it
//
int32_t CacheController::Read(void *dst, const void *src, size_t nBytesToRead) {
    uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(src);
    kMESIState state = kMesi_Exclusive;

    // can probably optimize a bit here - if the current line is exclusive - there is no need to broadcast!
    auto res = bus.BroadCastRead(idCore, addrDescriptor);
    if (res != kMesi_Invalid) {
        // Shared
        state = kMesi_Shared;
    }
    auto idxLine = ReadLine(addrDescriptor, state);

    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(src);

    return cache.CopyFromLine(dst, idxLine, offset, nBytesToRead);
}

//
// Write to cache - dst is a cached address, src is whatever the user supplied
//
int32_t CacheController::Write(void *dst, const void *src, size_t nBytesToWrite) {
    uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(dst);
    bus.BroadCastWrite(idCore, addrDescriptor);

    auto idxLine = ReadLine(addrDescriptor, kMESIState::kMesi_Exclusive);
    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(dst);

    return cache.CopyToLine(idxLine, offset, src, nBytesToWrite);

}

int32_t CacheController::ReadLine(uint64_t addrDescriptor, kMESIState state) {
    auto idxLine = cache.GetLineIndex(addrDescriptor);
    auto idxNext = cache.NextLineIndex();
    // Miss?
    if (idxLine < 0) {
        if (cache.GetLineState(idxNext) == kMesi_Modified) {
            WriteMemory(idxNext);
        }
        ReadMemory(idxNext, addrDescriptor, state);
        idxLine = idxNext;
    }
    return idxLine;
}
void CacheController::WriteMemory(int idxLine) {
    uint8_t tmp[GNK_L1_CACHE_LINE_SIZE];

    cache.ReadLineData(tmp, idxLine);
    bus.WriteMemory(cache.GetLineAddrDescriptor(idxLine), tmp);
}

void CacheController::ReadMemory(int idxLine, uint64_t addrDescriptor, kMESIState state) {
    uint8_t tmp[GNK_L1_CACHE_LINE_SIZE];
    bus.ReadMemory(tmp, addrDescriptor);
    cache.WriteLineData(idxLine, tmp, addrDescriptor, state);
}

void CacheController::Dump() {
    printf("CacheController, id=%d\n",idCore);
    cache.DumpCacheLines();
}

/*
int main() {
    int32_t value = 4711;
    uint8_t buffer[2048];
    RamMemory ram(65536);
    DataBus bus(ram);

    CacheController cacheControllerA(bus, 0);
    CacheController cacheControllerB(bus, 1);
    CacheController cacheControllerC(bus, 2);
    CacheController cacheControllerD(bus, 3);


    void *ptrRamDst = ram.TranslateAddress((void *)0x4711);
    auto nWritten = cacheControllerA.Write(ptrRamDst, &value, sizeof(value));
    printf("Wrote: %d bytes, value = %d\n", nWritten, value);

    int32_t outValue = 0;
    auto nRead = cacheControllerB.Read(&outValue, ptrRamDst, sizeof(value));
    printf("Read: %d bytes, got = %d\n", nRead, outValue);


    cacheControllerA.Dump();
    cacheControllerB.Dump();


    return 0;
}
 */
