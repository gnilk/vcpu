//
// Created by gnilk on 06.03.24.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include "Segment.h"

using namespace gnilk;
using namespace gnilk::assembler;

Segment::Segment(const std::string &segName) : name(segName) {
    // No chunks created here - this is used by the linker which merges the chunks from another segment..
}

Segment::Segment(const std::string &segName, uint64_t address) : name(segName), loadAddress(address) {
    CreateChunk(loadAddress);
}

void Segment::SetLoadAddress(uint64_t newLoadAddress) {
    currentChunk->SetLoadAddress(newLoadAddress);
}

void Segment::CopyFrom(const Segment::Ref other) {
    // FIXME: can this be a shallow copy???
    for(auto &ch : other->chunks) {
        chunks.push_back(ch);
    }
}
bool Segment::CreateChunk(uint64_t loadAddress) {
    // If the current one is empty - just change the load address and continue...
    // Don't do this - might be a bloody good reason someone is doing this...
    //if ((currentChunk!=nullptr) && currentChunk->IsEmpty()) {
    //    currentChunk->SetLoadAddress(loadAddress);
    //    return true;
   // }

    // FIXME: Verify that this load-address is not overlapping with existing chunks
    auto chunk = std::make_shared<Segment::DataChunk>();
    chunk->loadAddress = loadAddress;
    chunks.push_back(chunk);
    // FIXME: Sort chunks!!!
    std::sort(chunks.begin(), chunks.end(),[](const DataChunk::Ref &a, const DataChunk::Ref &b) {
       return (a->loadAddress < b->loadAddress);
    });
    currentChunk = chunk;
    return true;
}

const std::vector<Segment::DataChunk::Ref> &Segment::DataChunks() {
    return chunks;
}

Segment::DataChunk::Ref Segment::CurrentChunk() {
    return currentChunk;
}

Segment::DataChunk::Ref Segment::ChunkFromAddress(uint64_t address) {
    for(auto &chunk : chunks) {
        if ((address >= chunk->LoadAddress()) && (chunk->EndAddress() > address)) {
            return chunk;
        }
    }
    return nullptr;
}

// This should be enough if they are properly sorted!
uint64_t Segment::StartAddress() {
    return chunks[0]->loadAddress;
}

uint64_t Segment::EndAddress() {
    uint64_t endAddr = 0;
    for(auto &chunk : chunks) {
        if (chunk->EndAddress() > endAddr) {
            endAddr = chunk->EndAddress();
        }
    }
    return endAddr;
}

const std::string &Segment::Name() const {
    return name;
}

uint64_t Segment::GetCurrentWriteAddress() {
    if (currentChunk == nullptr) {
        return 0;
    }
    return currentChunk->GetCurrentWriteAddress();
};

size_t Segment::Write(const std::vector<uint8_t> &srcData) {
    if (currentChunk == nullptr) {
        return 0;
    }
    return currentChunk->Write(srcData);
}

size_t Segment::WriteByte(uint8_t byte) {
    if (currentChunk == nullptr) {
        return 0;
    }
    return currentChunk->WriteByte(byte);
}

void Segment::ReplaceAt(uint64_t offset, uint64_t newValue) {
    // 1) find segment related to this offset
    auto dstChunk = ChunkFromAddress(offset);
    if (dstChunk == nullptr) {
        int breakme = 1;
    }
    dstChunk->ReplaceAt(offset, newValue);
}

void Segment::ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize) {
    int breakme = 1;

}
//
// Segment::DataChunk impl
//

bool Segment::DataChunk::IsEmpty() const {
    return data.empty();
}

const std::vector<uint8_t> &Segment::DataChunk::Data() const {
    return data;
}

const void *Segment::DataChunk::DataPtr() const {
    return data.data();
}

size_t Segment::DataChunk::Size() const {
    return data.size();
}

uint64_t Segment::DataChunk::LoadAddress() const {
    return loadAddress;
}
uint64_t Segment::DataChunk::EndAddress() const {
    return loadAddress + data.size();
}


void Segment::DataChunk::SetLoadAddress(uint64_t newLoadAddress) {
    if (loadAddress == 100) {
        int breakme = 1;
    }
    loadAddress = newLoadAddress;
}


size_t Segment::DataChunk::WriteByte(uint8_t byte) {
    data.push_back(byte);
    return 1;
}
void Segment::DataChunk::ReplaceAt(uint64_t offset, uint64_t newValue) {
    if (data.size() < 8) {
        return;
    }
    if (offset > (data.size() - 8)) {
        return;
    }

    data[offset++] = (newValue>>56 & 0xff);
    data[offset++] = (newValue>>48 & 0xff);
    data[offset++] = (newValue>>40 & 0xff);
    data[offset++] = (newValue>>32 & 0xff);
    data[offset++] = (newValue>>24 & 0xff);
    data[offset++] = (newValue>>16 & 0xff);
    data[offset++] = (newValue>>8 & 0xff);
    data[offset++] = (newValue & 0xff);
}

void Segment::DataChunk::ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize) {
    if (data.size() < vcpu::ByteSizeOfOperandSize(opSize)) {
        return;
    }
    if (offset > (data.size() - vcpu::ByteSizeOfOperandSize(opSize))) {
        return;
    }
    switch(opSize) {
        case vcpu::OperandSize::Byte :
            data[offset++] = (newValue & 0xff);
            break;
        case vcpu::OperandSize::Word :
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            break;
        case vcpu::OperandSize::DWord :
            data[offset++] = (newValue>>24 & 0xff);
            data[offset++] = (newValue>>16 & 0xff);
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            break;
        default:
            return;
    }
}

size_t Segment::DataChunk::Write(const std::vector<uint8_t> &srcData) {
    data.insert(data.end(), srcData.begin(), srcData.end());
    return srcData.size();
}

uint64_t Segment::DataChunk::GetCurrentWriteAddress() {
    return loadAddress + data.size();
}
