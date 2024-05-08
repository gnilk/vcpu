//
// Created by gnilk on 06.03.24.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include "Segment.h"

using namespace gnilk;
using namespace gnilk::assembler;

Segment::Segment(kSegmentType segmentType) : type(segmentType) {
    // No chunks created here - this is used by the linker which merges the chunks from another segment..
}

Segment::Segment(kSegmentType segmentType, uint64_t address) : type(segmentType), loadAddress(address) {
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

   if (loadAddress == 0) {
       // Perhaps enable this warning later - once we have a proper warning-level system..

       // fmt::println(stderr, "Segment, creating chunk at address 0, switching to 'append' (which is zero for first chunk in a segment)");
       // fmt::println(stderr, "         if you really intended load-address 0, specify it with '.org 0x0000'");
       loadAddress = Segment::DataChunk::LOAD_ADDR_APPEND;
   }

    // FIXME: Verify that this load-address is not overlapping with existing chunks
    auto chunk = std::make_shared<Segment::DataChunk>();
    chunk->loadAddress = loadAddress;
    AddChunk(chunk);
    return true;
}

void Segment::AddChunk(DataChunk::Ref chunk) {
    chunks.push_back(chunk);
    // Sort chunks based on load-address
    // Note: If not load address given (i.e. no .org statement) then we just 'append' and the load address we compare to is 0
    std::sort(chunks.begin(), chunks.end(),[](const DataChunk::Ref &a, const DataChunk::Ref &b) {
        return (a->LoadAddress() < b->LoadAddress());
    });
    currentChunk = chunk;
}
const std::vector<Segment::DataChunk::Ref> &Segment::DataChunks() {
    return chunks;
}

Segment::DataChunk::Ref Segment::CurrentChunk() {
    return currentChunk;
}

std::pair<bool, Segment::kSegmentType> Segment::TypeFromString(const std::string &typeName) {
    if ((typeName == "code") || (typeName == ".code") || (typeName == "text") || (typeName == ".text")) {
        return {true, kSegmentType::Code};
    } else if ((typeName == "data") || (typeName == ".data")) {
        return {true, kSegmentType::Data};
    }
    return {false,{}};
}

const std::string &Segment::TypeName(Segment::kSegmentType type) {
    static std::unordered_map<Segment::kSegmentType, std::string> typeToName = {
            {kSegmentType::Data, "data"},
            {kSegmentType::Code, "code"},
    };
    return typeToName[type];
}

Segment::kSegmentType Segment::Type() const {
    return type;
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
    // In case empty - we assume 0
    if (chunks.empty()) {
        return 0;
    }

    return chunks[0]->LoadAddress();
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
    return Segment::TypeName(type);
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

bool Segment::DataChunk::Empty() const {
    return data.empty();
}

bool Segment::DataChunk::IsLoadAddressAppend() const {
    return (loadAddress == LOAD_ADDR_APPEND);
}

// THIS WILL TRANSLATE the loadAddress - DO NOT USE if you wish to check it...
uint64_t Segment::DataChunk::LoadAddress() const {
    if (loadAddress == LOAD_ADDR_APPEND) {
        return 0;
    }
    return loadAddress;
}
uint64_t Segment::DataChunk::EndAddress() const {
    return LoadAddress() + data.size();
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
bool Segment::DataChunk::ReplaceAt(uint64_t offset, uint64_t newValue) {
    if (data.size() < 8) {
        return false;
    }
    if (offset > (data.size() - 8)) {
        fmt::println(stderr, "DataChunk, out of range, trying to write to {} size={}", offset, data.size());
        return false;
    }

    data[offset++] = (newValue>>56 & 0xff);
    data[offset++] = (newValue>>48 & 0xff);
    data[offset++] = (newValue>>40 & 0xff);
    data[offset++] = (newValue>>32 & 0xff);
    data[offset++] = (newValue>>24 & 0xff);
    data[offset++] = (newValue>>16 & 0xff);
    data[offset++] = (newValue>>8 & 0xff);
    data[offset++] = (newValue & 0xff);

    return true;
}

bool Segment::DataChunk::ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize) {
    if (data.size() < vcpu::ByteSizeOfOperandSize(opSize)) {
        return false;
    }
    if (offset > (data.size() - vcpu::ByteSizeOfOperandSize(opSize))) {
        fmt::println(stderr, "DataChunk, out of range, trying to write to {} size={}", offset, data.size());
        return false;
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
        case vcpu::OperandSize::Long :
            data[offset++] = (newValue>>56 & 0xff);
            data[offset++] = (newValue>>48 & 0xff);
            data[offset++] = (newValue>>40 & 0xff);
            data[offset++] = (newValue>>32 & 0xff);
            data[offset++] = (newValue>>24 & 0xff);
            data[offset++] = (newValue>>16 & 0xff);
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            break;
    }
    return true;
}

size_t Segment::DataChunk::Write(const std::vector<uint8_t> &srcData) {
    data.insert(data.end(), srcData.begin(), srcData.end());
    return srcData.size();
}

uint64_t Segment::DataChunk::GetCurrentWriteAddress() {
    return LoadAddress() + data.size();
}
uint64_t Segment::DataChunk::GetRelativeWriteAddress() {
    return data.size();
}
