//
// Created by gnilk on 10.04.24.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Cache.h"

using namespace gnilk;
using namespace gnilk::vcpu;

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
