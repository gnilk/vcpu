#include <iostream>
#include <array>
#include <functional>
#include <string.h>
#include <unordered_map>

#include "System.h"
#include "CPUMemCache.h"
#include "FlashBus.h"

//
// Very small and simple MESI SMP cache coherency implementation
// see: https://en.wikipedia.org/wiki/MESI_protocol
//

using namespace gnilk::vcpu;


//////////////////////////////////////////////////////////////////////////
//
// Cache
//

int Cache::GetNumLines() const {
    return lines.size();
}

int Cache::GetLineIndex(uint64_t addrDescriptor) {

    // This is expensive - which is why there are n-way associative caches
    // In essence this is a 'direct mapped cache, in order to implement this differently
    // we should limit the search-space. For instance, split/hash the first look up (using some parts from the V-Addr)
    // then search the array. => n-set associative cache;
    // set = cachelineSets[hash(vAddr)];    // return search set; this is just an index * n (in n-set); like: index * 8
    // idxCacheLine = find_set_index(set, vAddr);   // find vAddr in set or not..
    //
    // A set is just another way to 'split' the cache-line array - so no need to reorganize the data...
    //
    //

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

kMESIState Cache::GetLineState(int idxLine) const {
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
size_t Cache::CopyFromLine(uint64_t dstAddress, int idxLine, uint16_t offset, size_t nBytes) {

    if ((offset + nBytes) > GNK_L1_CACHE_LINE_SIZE) {
        nBytes = GNK_L1_CACHE_LINE_SIZE - offset;
    }
    if (!nBytes) {
        return 0;
    }

    auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(dstAddress);
    bus->WriteLine(dstAddress, &lines[idxLine].data);
    return nBytes;
}

size_t Cache::CopyToLine(int idxLine, uint16_t offset, const uint64_t srcAddress, size_t nBytes) {
    if ((offset + nBytes) > GNK_L1_CACHE_LINE_SIZE) {
        nBytes = GNK_L1_CACHE_LINE_SIZE - offset;
    }
    if (!nBytes) {
        return 0;
    }

    auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(srcAddress);
    // FIXME: Assert bus != nullptr
    bus->ReadLine(lines[idxLine].data, srcAddress);
    // Update the cache line state...
    lines[idxLine].state = kMesi_Modified;
    return nBytes;
}

// Copies data from an 'external' (i.e. non-emulated pointer) to a cache line..
// This is used by the 'CacheController::Write' function...
int32_t Cache::CopyToLineFromExternal(int idxLine, uint16_t offset, const void *src, size_t nBytes) {
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

int32_t Cache::CopyFromLineToExternal(void *dst, int idxLine, uint16_t offset, size_t nBytes) {
    if ((offset + nBytes) > GNK_L1_CACHE_LINE_SIZE) {
        nBytes = GNK_L1_CACHE_LINE_SIZE - offset;
    }
    if (!nBytes) {
        return 0;
    }

    // This writes the actual data in the cache line to the RAM memory..
    memcpy(dst, &lines[idxLine].data[offset], nBytes);
    return nBytes;

}


void Cache::DumpCacheLines() const {
    for(int i=0;i<lines.size();i++) {
        printf("  %d  state=%s, time=%d, desc=0x%x\n",i, MESIStateToString(lines[i].state).c_str(), lines[i].time, (int)lines[i].addrDescriptor);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Cache Controller
//

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
    // Can't cache this - don't try...
    if (!SoC::Instance().IsAddressCacheable(address)) {
        return;
    }

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
    // Can't cache this - do it differently.. Need another bus..

    if (!SoC::Instance().IsAddressCacheable(address)) {
        auto baseBus = SoC::Instance().GetDataBusForAddressAs<FlashBus>(address);
        if (baseBus == nullptr) {
            return -1;
        }
        // FIXME: The non-cache-able bus should work on smaller values - 32bit?
        static uint8_t tmp[GNK_L1_CACHE_LINE_SIZE];
        memcpy(tmp, src, nBytes);
        baseBus->WriteData(address, src, nBytes);
        return (int32_t)nBytes;
    }

    // This can be cached - so proceed with the MESI bus...

    uint64_t dstAddrDesc = GNK_ADDR_DESC_FROM_ADDR(address);


    auto bus = SoC::Instance().GetDataBusForAddressAs<MesiBusBase>(address);
    bus->BroadCastWrite(idCore, dstAddrDesc);

    auto idxLine = ReadLine(bus, dstAddrDesc, kMESIState::kMesi_Exclusive);
    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(address);

    return cache.CopyToLineFromExternal(idxLine, offset, src, nBytes);
}

void CacheController::ReadInternalToExternal(void *dst, const uint64_t address, size_t nBytes) {
    // FIXME: Check if the region for the address can be cached!!
    if (!SoC::Instance().IsAddressCacheable(address)) {
        // Read non-cache-able regional value..
        // But must be different..

        auto baseBus = SoC::Instance().GetDataBusForAddressAs<FlashBus>(address);
        if (baseBus == nullptr) {
            return;
        }
        baseBus->ReadData(dst, address, nBytes);
        return;
    }


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

//
// Read from cache - 'src' is a cache-line address, dst is somewhere the user wants it
//
/*
int32_t CacheController::ReadInternal(uint64_t dstAddress, const uint64_t srcAddress, size_t nBytesToRead) {
    uint64_t addrDescriptor = GNK_ADDR_DESC_FROM_ADDR(srcAddress);
    kMESIState state = kMesi_Exclusive;

    // can probably optimize a bit here - if the current line is exclusive - there is no need to broadcast!
    auto srcBus = SoC::Instance().GetDataBusForAddress(addrDescriptor);

    auto res = srcBus->BroadCastRead(idCore, addrDescriptor);
    if (res != kMesi_Invalid) {
        // Shared
        state = kMesi_Shared;
    }
    auto idxLine = ReadLine(srcBus, addrDescriptor, state);

    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(srcAddress);
    return cache.CopyFromLine(dstAddress, idxLine, offset, nBytesToRead);
}

//
// Write to cache - dst is a cached address, src is whatever the user supplied
//
int32_t CacheController::WriteInternal(uint64_t dstAddress, const uint64_t srcAddress, size_t nBytesToWrite) {
    uint64_t dstAddrDesc = GNK_ADDR_DESC_FROM_ADDR(dstAddress);
    auto dstBus = SoC::Instance().GetDataBusForAddress(dstAddress);

    dstBus->BroadCastWrite(idCore, dstAddrDesc);
    auto idxLine = ReadLine(dstBus, dstAddrDesc, kMESIState::kMesi_Exclusive);
    uint16_t offset = GNK_LINE_OFS_FROM_ADDR(dstAddress);

    return cache.CopyToLine(idxLine, offset, srcAddress, nBytesToWrite);

}
*/

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


