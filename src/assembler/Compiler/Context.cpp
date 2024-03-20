//
// Created by gnilk on 07.03.24.
//

#include "Context.h"

using namespace gnilk;
using namespace gnilk::assembler;

void Context::Clear() {
    segments.clear();
    activeSegment = nullptr;
    // FIXME: Clear list of compiled units..
}

void Context::AddStructDefinition(const StructDefinition &structDefinition) {
    structDefinitions.push_back(structDefinition);
}
bool Context::HasConstant(const std::string &name) {
    return constants.contains(name);
}
void Context::AddConstant(const std::string &name, ast::ConstLiteral::Ref constant) {
    constants[name] = std::move(constant);
}
ast::ConstLiteral::Ref Context::GetConstant(const std::string &name) {
    return constants[name];
}


bool Context::HasStructDefinintion(const std::string &typeName) {
    for(auto &structDef : structDefinitions) {
        if (structDef.ident == typeName) {
            return true;
        }
    }
    return false;
}
const StructDefinition &Context::GetStructDefinitionFromTypeName(const std::string &typeName) {
    for(auto &structDef : structDefinitions) {
        if (structDef.ident == typeName) {
            return structDef;
        }
    }
    // Should never be reached - throw something???
    fmt::println(stderr, "Compiler, critical error; no struct with type name={}", typeName);
    exit(1);
}
bool Context::HasIdentifierAddress(const std::string &ident) {
    return identifierAddresses.contains(ident);
}
void Context::AddIdentifierAddress(const std::string &ident, const IdentifierAddress &idAddress) {
    identifierAddresses[ident] = idAddress;
}
IdentifierAddress &Context::GetIdentifierAddress(const std::string &ident) {
    return identifierAddresses[ident];
}

void Context::AddAddressPlaceholder(const IdentifierAddressPlaceholder::Ref &addressPlaceholder) {
    addressPlaceholders.push_back(addressPlaceholder);
}


size_t Context::Write(const std::vector<uint8_t> &data) {
    auto nWritten = unit.Write(*this, data);
    return nWritten;
}


void Context::Merge() {
    // Make sure this is empty...
    outputdata.clear();
    MergeAllSegments(outputdata);
}

// FIXME: Temporary
void Context::MergeAllSegments(std::vector<uint8_t> &out) {
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



//
bool Context::EnsureChunk() {
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
bool Context::GetOrAddSegment(const std::string &name, uint64_t address) {
    if (!segments.contains(name)) {
        auto segment = std::make_shared<Segment>(name, address);
        segments[name] = segment;
    }
    activeSegment = segments.at(name);
    return true;
}

bool Context::CreateEmptySegment(const std::string &name) {
    auto segment = std::make_shared<Segment>(name);
    segments[name] = segment;
    activeSegment = segment;
    return true;
}

bool Context::SetActiveSegment(const std::string &name) {
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
uint64_t Context::GetCurrentWriteAddress() {
    if (activeSegment == nullptr) {
        fmt::println(stderr, "Compiler, not active segment!!");
        return 0;
    }
    if (activeSegment->currentChunk == nullptr) {
        fmt::println(stderr, "Compiler, no chunk in active segment {}", activeSegment->Name());
        return 0;
    }
    return activeSegment->currentChunk->GetCurrentWriteAddress();
}

bool Context::HaveSegment(const std::string &name) {
    return segments.contains(name);
}

const Segment::Ref Context::GetSegment(const std::string segName) {
    if (!segments.contains(segName)) {
        return nullptr;
    }
    return segments.at(segName);
}

size_t Context::GetSegments(std::vector<Segment::Ref> &outSegments) {
    for(auto [k, v] : segments) {
        outSegments.push_back(v);
    }
    return outSegments.size();
}


Segment::Ref Context::GetActiveSegment() {
    return activeSegment;
}


size_t Context::GetSegmentEndAddress(const std::string &name) {
    if (!segments.contains(name)) {
        return 0;
    }
    return segments.at(name)->EndAddress();
}

