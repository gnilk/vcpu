//
// Created by gnilk on 07.03.24.
//

//
// TO-DO:
// - MUCH better support for multiple compile units
//   - Check if .org statements overlap within segment (in case multiple compile units have segment and org statements causing overlaps)
//   + handle symbol export -> this needs full lexer/ast/compiler support as well - but here we need
//     - symbols across units => context
//     - local symbols => unit (or name-mangling - think namespaces)
//     [note: the elf write will only care about symbols in the context]
//

#include "Context.h"
#include "CompiledUnit.h"

using namespace gnilk;
using namespace gnilk::assembler;

void Context::Clear() {
    segments.clear();
    units.clear();
    activeSegment = nullptr;
}

CompiledUnit &Context::CreateUnit() {
    units.emplace_back();
    return units.back();
}

void Context::AddStructDefinition(const StructDefinition &structDefinition) {
    structDefinitions.push_back(structDefinition);
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
const std::vector<StructDefinition> &Context::StructDefinitions() {
    return structDefinitions;
}


bool Context::HasExport(const std::string &ident) {
    return publicIdentifiers.contains(ident);
}

void Context::AddExport(const std::string &ident) {
    publicIdentifiers[ident] = {};
}

Identifier &Context::GetExport(const std::string &ident) {
    return publicIdentifiers[ident];
}



size_t Context::Write(const std::vector<uint8_t> &data) {
    if (!EnsureChunk()) {
        return 0;
    }
    fmt::println(stderr, "Context::Write - not yet implemented!");
    exit(1);
    //auto nWritten = CurrentUnit().Write(*this, data);
    //return nWritten;
    return 0;
}

void Context::Merge() {
    // Make sure this is empty...
    outputdata.clear();
    MergeAllSegments(outputdata);
}

// FIXME: Move to linker...
void Context::MergeAllSegments(std::vector<uint8_t> &out) {
    size_t startAddress = 0;
    size_t endAddress = 0;

    // Merge segments across units and copy data...
    for(auto &unit : units) {
        std::vector<Segment::Ref> unitSegments;
        unit.GetSegments(unitSegments);
        for(auto srcSeg : unitSegments) {
            GetOrAddSegment(srcSeg->Name());
            for(auto srcChunk : srcSeg->chunks) {
                if (srcChunk->LoadAddress() == 0) {
                    activeSegment->CreateChunk(activeSegment->EndAddress());
                } else {
                    activeSegment->CreateChunk(srcChunk->LoadAddress());
                }
                activeSegment->currentChunk->Write(srcChunk->Data());
            }
        }
    }

    // Compute full binary end address...
    for(auto [name, seg] : segments) {
        if (seg->EndAddress() > endAddress) {
            endAddress = seg->EndAddress();
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
bool Context::GetOrAddSegment(const std::string &name) {
    if (!segments.contains(name)) {
        auto segment = std::make_shared<Segment>(name);
        segments[name] = segment;
    }
    activeSegment = segments.at(name);
    return true;
}

bool Context::CreateEmptySegment(const std::string &name) {
    if (HaveSegment(name)) {
        activeSegment = segments[name];
        return true;
    }

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
        // if no chunk - we ship back zero - as we will create one on first write...
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

