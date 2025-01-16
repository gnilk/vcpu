//
// Created by gnilk on 10.04.24.
//
//
// Very small and simple MESI SMP cache coherency implementation
// see: https://en.wikipedia.org/wiki/MESI_protocol
//

#include "System.h"
#include "CacheController.h"

using namespace gnilk::vcpu;

//
// TODO: Should rewrite so we don't depend on the SOC instance
// - Initialize to take 'std::vector<MemoryRegion *> &regions' and a address-to-bus translation routine
//

// Note: This should not go to the CTOR as we call the global SoC object - subject to change
void CacheController::Initialize(uint8_t coreIdentifier) {
    idCore = coreIdentifier;

    // FIXME: Not quite sure I want this to automatically map cacheable regions, better to supply them in the initializer
    std::vector<MemoryRegion *> regions;
    SoC::Instance().GetCacheableRegions(regions);

    // Let's assume we subscribe to all cacheable regions...
    for(auto r : regions) {
        if ((r->bus != nullptr) && (r->flags & kRegionFlag_Cache)) {
            r->bus->Subscribe(idCore, [this](BusBase::kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
                return OnDataBusMessage(op, sender, addrDescriptor);
            });
        }
    }
}

kMESIState CacheController::OnDataBusMessage(BusBase::kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
    switch(op) {
        case MesiBusBase::kMemOp::kBusRd :
            return OnMsgBusRd(addrDescriptor);
        case MesiBusBase::kMemOp::kBusWr :
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
        auto bus = SoC::Instance().GetDataBusForAddress(addrDescriptor);
        WriteMemory(bus, idxLine);
    }
    return cache.SetLineState(idxLine, kMesi_Shared);
}

void CacheController::OnMsgBusWr(uint64_t addrDescriptor) {
    auto idxLine = cache.GetLineIndex(addrDescriptor);

    if (idxLine < 0) {
        return;
    }

    if (cache.GetLineState(idxLine) == kMesi_Modified) {
        auto bus = SoC::Instance().GetDataBusForAddress(addrDescriptor);
        WriteMemory(bus, idxLine);
        cache.ResetLine(idxLine);
    }
}

// Touch will ensure is in the cache
void CacheController::Touch(const uint64_t address) {
    uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(address);
    kMESIState state = kMesi_Exclusive;

    auto bus = SoC::Instance().GetDataBusForAddress(address);

    auto res = bus->BroadCastRead(idCore, addrDescriptor);
    if (res != kMesi_Invalid) {
        // Shared
        state = kMesi_Shared;
    }
    ReadLine(bus, addrDescriptor, state);
}

// Private - this is called from 'Write<T>' - which is a wrapper so we can copy absolute values to the
// emulate cache RAM...
int32_t CacheController::WriteInternalFromExternal(uint64_t address, const void *src, size_t nBytes) {
    // we can span multiple cache-lines since we allow unaligned access!
    auto *ptrSrcData = const_cast<uint8_t *>(static_cast<const uint8_t *>(src));    // I really dislike C++ sometimes...
    size_t nLeft = nBytes;
    while(nLeft) {
        uint64_t dstAddrDesc = GNK_ADDR_DESC_FROM_ADDR(address);

        auto bus = SoC::Instance().GetDataBusForAddress(address);
        bus->BroadCastWrite(idCore, dstAddrDesc);

        auto idxLine = ReadLine(bus, dstAddrDesc, kMESIState::kMesi_Exclusive);
        uint16_t offset = GNK_LINE_OFS_FROM_ADDR(address);

        auto nWritten = cache.CopyToLineFromExternal(idxLine, offset, ptrSrcData, nLeft);
        nLeft -= nWritten;
        if (!nLeft) break;
        ptrSrcData += nWritten;
        address += nWritten;
    }
    // cast doesn't matter..
    return (int32_t)nBytes;
}

void CacheController::ReadInternalToExternal(void *dst, const uint64_t address, size_t nBytes) {
    auto *ptrDstData = static_cast<uint8_t *>(dst);    // I really dislike C++ sometimes...
    auto readAddress = address;
    size_t nLeft = nBytes;
    // Must be done in a loop since we allow unaligned read's
    // Ergo, if a full read would cross the cache-line boundary - we need to split the read in two...
    while(nLeft) {

        kMESIState state = kMesi_Exclusive;
        uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(readAddress);

        // can probably optimize a bit here - if the current line is exclusive - there is no need to broadcast!
        auto bus = SoC::Instance().GetDataBusForAddress(readAddress);
        auto res = bus->BroadCastRead(idCore, addrDescriptor);
        if (res != kMesi_Invalid) {
            // Shared
            state = kMesi_Shared;
        }
        // pass the bus here - avoid lookup twice...
        auto idxLine = ReadLine(bus, addrDescriptor, state);

        uint16_t offset = GNK_LINE_OFS_FROM_ADDR(readAddress);
        auto nRead = cache.CopyFromLineToExternal(ptrDstData, idxLine, offset, nLeft);
        nLeft -= nRead;
        if(!nLeft) break;
        ptrDstData += nRead;
        readAddress += nRead;
    }
}


int32_t CacheController::ReadLine(BusBase::Ref bus, uint64_t addrDescriptor, kMESIState state) {
    auto idxLine = cache.GetLineIndex(addrDescriptor);
    auto idxNext = cache.NextLineIndex();
    // Miss?
    if (idxLine < 0) {
        if (cache.GetLineState(idxNext) == kMesi_Modified) {
            WriteMemory(bus, idxNext);
        }
        ReadMemory(bus, idxNext, addrDescriptor, state);
        idxLine = idxNext;
    }
    return idxLine;
}

size_t CacheController::Flush() {
    size_t nLinesFlushed = 0;
    for (auto i = 0; i<cache.GetNumLines();i++) {
        if (cache.GetLineState(i) == kMesi_Modified) {
            // All lines in the cache MUST come from a MESI compatible bus...
            auto bus = SoC::Instance().GetDataBusForAddress(cache.lines[i].addrDescriptor);
            WriteMemory(bus, i);
            nLinesFlushed++;
        }
        // Need the reset call here otherwise cached but not modified lines will still be present
        cache.ResetLine(i);
    }
    return nLinesFlushed;
}

void CacheController::WriteMemory(const BusBase::Ref &bus, int idxLine) {
    uint8_t tmp[GNK_L1_CACHE_LINE_SIZE];

    cache.ReadLineData(tmp, idxLine);
    bus->WriteLine(cache.GetLineAddrDescriptor(idxLine), tmp);
}

void CacheController::ReadMemory(const BusBase::Ref &bus, int idxLine, uint64_t addrDescriptor, kMESIState state) {
    uint8_t tmp[GNK_L1_CACHE_LINE_SIZE];
    bus->ReadLine(tmp, addrDescriptor);
    cache.WriteLineData(idxLine, tmp, addrDescriptor, state);
}

void CacheController::Dump() const {
    printf("CacheController, id=%d\n",idCore);
    cache.DumpCacheLines();
}
int CacheController::GetInvalidLineCount() const {
    int nInvalid = 0;
    auto nLines = cache.GetNumLines();
    for(auto i=0;i<nLines;i++) {
        auto state = cache.GetLineState(i);
        if (state & kMesi_Invalid) {
            nInvalid++;
        }
    }
    return nInvalid;
}


