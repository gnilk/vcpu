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

// This should not be in the CTOR - I hate CTOR's...
void CacheController::Initialize(uint8_t coreIdentifier) {
    idCore = coreIdentifier;


    std::vector<MemoryRegion *> regions;
    SoC::Instance().GetCacheableRegions(regions);

    // Let's assume we subscribe to all cacheable regions...
    for(auto r : regions) {
        if ((r->bus != nullptr) && (r->flags & kRegionFlag_Cache)) {
            auto mesiBus = std::reinterpret_pointer_cast<MesiBusBase>(r->bus);
            mesiBus->Subscribe(idCore, [this](MesiBusBase::kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
                return OnDataBusMessage(op, sender, addrDescriptor);
            });
        }
    }
}

kMESIState CacheController::OnDataBusMessage(MesiBusBase::kMemOp op, uint8_t sender, uint64_t addrDescriptor) {
    // printf("CacheController::OnDataBusMessage\n");

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
        auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(addrDescriptor);
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
        auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(addrDescriptor);
        WriteMemory(bus, idxLine);
        cache.ResetLine(idxLine);
    }
}

// Touch will ensure is in the cache
void CacheController::Touch(const uint64_t address) {
    uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(address);
    kMESIState state = kMesi_Exclusive;

    auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(address);

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
    uint64_t dstAddrDesc = GNK_ADDR_DESC_FROM_ADDR(address);

    auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(address);
    bus->BroadCastWrite(idCore, dstAddrDesc);

    auto idxLine = ReadLine(bus, dstAddrDesc, kMESIState::kMesi_Exclusive);
    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(address);

    return cache.CopyToLineFromExternal(idxLine, offset, src, nBytes);
}

void CacheController::ReadInternalToExternal(void *dst, const uint64_t address, size_t nBytes) {
    kMESIState state = kMesi_Exclusive;
    uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(address);

    // can probably optimize a bit here - if the current line is exclusive - there is no need to broadcast!
    auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(address);
    auto res = bus->BroadCastRead(idCore, addrDescriptor);
    if (res != kMesi_Invalid) {
        // Shared
        state = kMesi_Shared;
    }
    // pass the bus here - avoid lookup twice...
    auto idxLine = ReadLine(bus, addrDescriptor, state);

    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(address);
    cache.CopyFromLineToExternal(dst, idxLine, offset, nBytes);
}


int32_t CacheController::ReadLine(MesiBusBase::Ref bus, uint64_t addrDescriptor, kMESIState state) {
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

void CacheController::Flush() {
    for (auto i = 0; i<cache.GetNumLines();i++) {
        if (cache.GetLineState(i) == kMesi_Modified) {
            // All lines in the cache MUST come from a MESI compatible bus...
            auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(cache.lines[i].addrDescriptor);
            WriteMemory(bus, i);
            cache.ResetLine(i);
        }
    }
}

void CacheController::WriteMemory(MesiBusBase::Ref bus, int idxLine) {
    uint8_t tmp[GNK_L1_CACHE_LINE_SIZE];

    cache.ReadLineData(tmp, idxLine);
    bus->WriteLine(cache.GetLineAddrDescriptor(idxLine), tmp);
}

void CacheController::ReadMemory(MesiBusBase::Ref bus, int idxLine, uint64_t addrDescriptor, kMESIState state) {
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

