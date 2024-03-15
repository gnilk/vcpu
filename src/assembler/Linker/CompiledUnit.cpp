//
// Created by gnilk on 20.12.23.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include "Linker/CompiledUnit.h"

using namespace gnilk;
using namespace gnilk::assembler;


// Compiled Unit
void CompiledUnit::Clear() {
    segments.clear();
}

bool CompiledUnit::EnsureChunk() {
    if (activeSegment == nullptr) {
        fmt::println(stderr, "Compiler, need at least an active segment");
        return false;
    }
    if (activeSegment->currentChunk != nullptr) {
        return true;
    }
    activeSegment->CreateChunk(GetCurrentWriteAddress());
    return true;
}

size_t CompiledUnit::Write(const std::vector<uint8_t> &data) {
    if (activeSegment == nullptr) {
        return 0;
    }
    if (activeSegment->currentChunk == nullptr) {
        return 0;
    }
    auto nWritten = activeSegment->currentChunk->Write(data);

    currentWriteAddress = activeSegment->CurrentChunk()->GetCurrentWriteAddress();
    return nWritten;
}


bool CompiledUnit::GetOrAddSegment(const std::string &name, uint64_t address) {
    if (!segments.contains(name)) {
        auto segment = std::make_shared<Segment>(name, address);
        segments[name] = segment;
    }
    activeSegment = segments.at(name);
    return true;
}
bool CompiledUnit::CreateEmptySegment(const std::string &name) {
    auto segment = std::make_shared<Segment>(name);
    segments[name] = segment;
    activeSegment = segment;
    return true;
}

bool CompiledUnit::SetActiveSegment(const std::string &name) {
    if (!segments.contains(name)) {
        return false;
    }
    activeSegment = segments.at(name);
    if (activeSegment->currentChunk == nullptr) {
        if (activeSegment->chunks.empty()) {
            fmt::println(stderr, "Linker, Compiled unit can't link - no data!");
            return false;
        }
        activeSegment->currentChunk =  activeSegment->chunks[0];
    }
    return true;
}

bool CompiledUnit::HaveSegment(const std::string &name) {
    return segments.contains(name);
}

const Segment::Ref CompiledUnit::GetSegment(const std::string segName) {
    if (!segments.contains(segName)) {
        return nullptr;
    }
    return segments.at(segName);
}


Segment::Ref CompiledUnit::GetActiveSegment() {
    return activeSegment;
}


size_t CompiledUnit::GetSegmentEndAddress(const std::string &name) {
    if (!segments.contains(name)) {
        return 0;
    }
    return segments.at(name)->EndAddress();
}



void CompiledUnit::SetBaseAddress(uint64_t address) {
    baseAddress = address;
}

uint64_t CompiledUnit::GetBaseAddress() {
    return baseAddress;
}


bool CompiledUnit::WriteByte(uint8_t byte) {
    if (!activeSegment) {
        return false;
    }
    if (!activeSegment->WriteByte(byte)) {
        return false;
    }
    currentWriteAddress = activeSegment->currentChunk->GetCurrentWriteAddress();
    return true;
}
void CompiledUnit::ReplaceAt(uint64_t offset, uint64_t newValue) {
    if (!activeSegment) {
        return;
    }
    activeSegment->ReplaceAt(offset, newValue);
}
void CompiledUnit::ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize) {
    if (!activeSegment) {
        return;
    }
    activeSegment->ReplaceAt(offset, newValue, opSize);
}



uint64_t CompiledUnit::GetCurrentWriteAddress() {
    return currentWriteAddress;
}

const std::vector<uint8_t> &CompiledUnit::Data() {
    if (!GetOrAddSegment("link_out", 0x00)) {
        static std::vector<uint8_t> empty = {};
        return empty;
    }
    return activeSegment->CurrentChunk()->Data();
}

size_t CompiledUnit::GetSegments(std::vector<Segment::Ref> &outSegments) {
    for(auto [k, v] : segments) {
        outSegments.push_back(v);
    }
    return outSegments.size();
}

// THIS IS DEPRECATED!
bool CompiledUnit::MergeSegments(const std::string &dst, const std::string &src) {
    // The source must exists
    if (!segments.contains(src)) {
        return false;
    }
    auto srcSeg = segments.at(src);
    if (!segments.contains(dst)) {
        if (!CreateEmptySegment(dst)) {
            return false;
        }
    }
    auto dstSeg = segments.at(dst);
    // FIXME: This is definitley not true!!!
    // first merge is just a plain copy...
    dstSeg->CopyFrom(srcSeg);
//    dstSeg->data.insert(dstSeg->data.end(), srcSeg->data.begin(), srcSeg->data.end());
    return true;
}

// FIXME: Temporary
void CompiledUnit::MergeAllSegments(std::vector<uint8_t> &out) {
    // FIXME: This should be an helper
    size_t startAddress = 0;
    size_t endAddress = 0;
    for(auto [name, seg] : segments) {
        for(auto chunk : seg->chunks) {
            if (chunk->EndAddress() > endAddress) {
                endAddress = chunk->EndAddress();
            }
        }
    }

    // This should be done in the linker

    // Calculate the size of final binary imasge
    auto szFinalData = endAddress - startAddress;
    out.resize(szFinalData);

    // Append the data
    for(auto [name, seg] : segments) {
        for(auto chunk : seg->chunks) {
            memcpy(out.data()+chunk->LoadAddress(), chunk->DataPtr(), chunk->Size());
        }
    }
}