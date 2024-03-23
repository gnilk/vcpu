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
#include "CompileUnit.h"

using namespace gnilk;
using namespace gnilk::assembler;

static void VectorReplaceAt(std::vector<uint8_t> &data, uint64_t offset, int64_t newValue, vcpu::OperandSize opSize);


void Context::Clear() {
    segments.clear();
    units.clear();
    activeSegment = nullptr;
}

CompileUnit &Context::CreateUnit() {
    units.emplace_back();
    return units.back();
}

void Context::AddStructDefinition(const StructDefinition &structDefinition) {
    structDefinitions.push_back(std::make_shared<StructDefinition>(structDefinition));
}

bool Context::HasStructDefinintion(const std::string &typeName) {
    for(auto &structDef : structDefinitions) {
        if (structDef->ident == typeName) {
            return true;
        }
    }
    return false;
}
const StructDefinition::Ref Context::GetStructDefinitionFromTypeName(const std::string &typeName) {
    for(auto structDef : structDefinitions) {
        if (structDef->ident == typeName) {
            return structDef;
        }
    }
    // Should never be reached - throw something???
    return nullptr;
}
const std::vector<StructDefinition::Ref> &Context::StructDefinitions() {
    return structDefinitions;
}


bool Context::HasExport(const std::string &ident) {
    return exportIdentifiers.contains(ident);
}

void Context::AddExport(const std::string &ident) {

    // I would like to do '{ .name = ident }' but I can't... not sure why...  can't manage to find a google to match my need...
    if (exportIdentifiers.contains(ident)) {
        auto expIdent = exportIdentifiers[ident];
        if (expIdent->origIdentifier != nullptr) {
            fmt::println(stderr, "Compiler, export {} already exists!", ident);
            return;
        }
        if (expIdent->name.empty()) {
            expIdent->name = ident;
        }
        return;
    }

    auto exportIdentifier = std::make_shared<ExportIdentifier>();
    exportIdentifier->name = ident;

    exportIdentifiers[ident] = exportIdentifier;
}

ExportIdentifier::Ref Context::GetExport(const std::string &ident) {
    // This is a forward declaration, we don't have it in the public x-unit list, so we create it
    // will set the 'origIdentifier' in the Export to nullptr which should be filled out when merging units
    if (!exportIdentifiers.contains(ident)) {
        fmt::println("Compiler, forward declare export '{}'", ident);
        AddExport(ident);
    }
    return exportIdentifiers[ident];
}

size_t Context::Write(const std::vector<uint8_t> &data) {
    if (!EnsureChunk()) {
        return 0;
    }
    return activeSegment->currentChunk->Write(data);
}

void Context::Merge() {
    // Make sure this is empty...
    outputData.clear();
    MergeAllSegments(outputData);
}

void Context::MergeAllSegments(std::vector<uint8_t> &out) {
    size_t startAddress = 0;
    size_t endAddress = 0;

    size_t unitCounter = 0;

    // Merge segments across units and copy data...
    for(auto &unit : units) {
        fmt::println("Merge segments in unit {}", unitCounter);
        unitCounter += 1;

        std::vector<Segment::Ref> unitSegments;
        unit.GetSegments(unitSegments);
        for(auto srcSeg : unitSegments) {
            // Make sure we have this segment
            GetOrAddSegment(srcSeg->Name());
            for(auto srcChunk : srcSeg->chunks) {
                // Create out own chunk at the correct address - chunk merging happen later...
                if (srcChunk->IsLoadAddressAppend()) {
                    // No load-address given  - just append
                    // FIXME: We should check overlap here!
                    activeSegment->CreateChunk(activeSegment->EndAddress());

                } else {
                    // Load address given, create chunk at this specific load address
                    // FIXME: We should check overlap here!
                    activeSegment->CreateChunk(srcChunk->LoadAddress());
                }
                ReloacteChunkFromUnit(unit, srcChunk);
            }
        }
    }

    // Compute full binary end address...
    for(auto [name, seg] : segments) {
        if (seg->EndAddress() > endAddress) {
            endAddress = seg->EndAddress();
        }
    }

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

// Relocates the active chunk from unit::<seg>::chunk
void Context::ReloacteChunkFromUnit(CompileUnit &unit, Segment::DataChunk::Ref srcChunk) {
    // Write data to it...
    Write(srcChunk->Data());

    // relocate local variables within this chunk...
    auto loadAddress = activeSegment->currentChunk->LoadAddress();
    for (auto &[symbol, identifier] : unit.GetIdentifiers()) {
        // Relocate all within this chunk...
        if (identifier->chunk != srcChunk) {
            continue;
        }
        if(identifier->resolvePoints.empty()) {
            continue;
        }

        fmt::println("  {} @ {:#x}", symbol, identifier->absoluteAddress);
        for(auto &resolvePoint : identifier->resolvePoints) {
            fmt::println("    - {:#x}",resolvePoint.placeholderAddress);
            if (!resolvePoint.isRelative) {
                activeSegment->currentChunk->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, identifier->absoluteAddress, resolvePoint.opSize);
            } else {
                int64_t offset = static_cast<int64_t>(identifier->absoluteAddress) - static_cast<int64_t>(resolvePoint.placeholderAddress);
                if (offset < 0) {
                    offset -= 1;
                }
                activeSegment->currentChunk->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, offset, resolvePoint.opSize);
            }
        }
    }

    // Update resolve points for any public identifier (export) belonging to this chunk
    // Since the chunk might have moved - the resolvePoint (i.e. the point to be modified when resolving identifiers) might have moved
    // So we need to adjust all of them...
    for(auto &[symbol, identifier] : exportIdentifiers) {
        // Do we have resolve points?
        if (identifier->resolvePoints.empty()) {
            continue;
        }

        // Loop through them and find any belonging to this chunk...
        for(auto &resolvePoint : identifier->resolvePoints) {
            // exports can't have relative resolve points
            if (resolvePoint.isRelative) {
                fmt::println(stderr, "Compiler, exports are not allowed relative lookup addresses");
                continue;
            }
            if (srcChunk != resolvePoint.chunk) {
                continue;
            }
            auto srcLoad = srcChunk->LoadAddress();
            auto dstLoad = activeSegment->currentChunk->LoadAddress();
            auto delta = static_cast<int64_t>(dstLoad) - static_cast<int64_t>(srcLoad);
            // Compute the new placeholder address...
            auto newPlaceHolderAddress = resolvePoint.placeholderAddress + delta;
            fmt::println("  srcLoad={}, dstLoad={}, delta={}, oldPlacedHolder={}, newPlaceHolder={}", srcLoad, dstLoad, delta, resolvePoint.placeholderAddress, newPlaceHolderAddress);

            // Move the placeHolder address accordingly
            resolvePoint.placeholderAddress = newPlaceHolderAddress;
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



////////
static void VectorReplaceAt(std::vector<uint8_t> &data, uint64_t offset, int64_t newValue, vcpu::OperandSize opSize) {
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
        case vcpu::OperandSize::Long :
            data[offset++] = (newValue>>56 & 0xff);
            data[offset++] = (newValue>>48 & 0xff);
            data[offset++] = (newValue>>40 & 0xff);
            data[offset++] = (newValue>>32 & 0xff);
            data[offset++] = (newValue>>24 & 0xff);
            data[offset++] = (newValue>>16 & 0xff);
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            return;
    }
}
