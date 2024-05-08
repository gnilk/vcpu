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

#include <algorithm>
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
ExportIdentifier::Ref Context::GetExport(const std::string &ident) const {
    // This is a forward declaration, we don't have it in the public x-unit list, so we create it
    // will set the 'origIdentifier' in the Export to nullptr which should be filled out when merging units
    if (!exportIdentifiers.contains(ident)) {
        fmt::println("Compiler, forward declare export '{}'", ident);
        return nullptr;
    }
    return exportIdentifiers.at(ident);
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

// Hmm - perhaps repurpose this one...
void Context::MergeAllSegments(std::vector<uint8_t> &out) {
    size_t startAddress = 0;
    size_t endAddress = 0;

    size_t unitCounter = 0;

    // Merge segments across units and copy data...
    for(auto &unit : units) {
        fmt::println("Merge segments in unit {}", unitCounter);
        unitCounter += 1;
        MergeSegmentForUnit(unit, Segment::kSegmentType::Code);
        MergeSegmentForUnit(unit, Segment::kSegmentType::Data);
    }
    // Dump all chunks
    for(auto seg : segments) {
        fmt::println("segment: {}", Segment::TypeName(seg->Type()));
        for(auto chunk : seg->chunks) {
            fmt::println("  chunks, start={}, end={}", chunk->LoadAddress(), chunk->EndAddress());
        }
    }

    // This can only be done after merging...
    endAddress = ComputeEndAddress();

    // Calculate the size of final binary image
    auto szFinalData = endAddress - startAddress;
    out.resize(szFinalData);

    // Append the data
    for(auto &seg : segments) {
        for(auto chunk : seg->chunks) {
            // Let's check if we can fit here - other this is a fatal bug...
            if ((chunk->LoadAddress() + chunk->Size()) > out.size()) {
                fmt::println(stderr, "Linker, in Context; final serialize would overflow data buffer - abort!");
                fmt::println(stderr, "  outdata.size() = {} bytes",out.size());
                fmt::println(stderr, "  chunk; loadAddress = {}, size={} bytes",chunk->LoadAddress(), chunk->Size());
                exit(1);
            }
            memcpy(out.data()+chunk->LoadAddress(), chunk->DataPtr(), chunk->Size());
        }
    }
}
size_t Context::ComputeEndAddress() {
    size_t endAddress = 0;
    auto codeSeg = GetSegment(Segment::kSegmentType::Code);
    if (codeSeg != nullptr) {
        endAddress = codeSeg->EndAddress();
    }
    auto dataSeg = GetSegment(Segment::kSegmentType::Data);
    if (dataSeg != nullptr) {
        // This is what we need for data..
        endAddress += (dataSeg->EndAddress() - dataSeg->StartAddress()) + dataSeg->StartAddress();
    }
    return endAddress;
}

// Merge
void Context::MergeSegmentForUnit(CompileUnit &unit, Segment::kSegmentType segType) {
    auto srcSeg = unit.GetSegment(segType);
    if (srcSeg == nullptr) {
        return;
    }

    // Assuming we 'append' here...
    // Current default start equals current end...
    auto startAddress = ComputeEndAddress();

    // Add to our own context...
    GetOrAddSegment(srcSeg->Type());
    for(auto srcChunk : srcSeg->chunks) {
        // Create out own chunk at the correct address - chunk merging happen later...
        if (srcChunk->IsLoadAddressAppend()) {
            // No load-address given  - just append
            // FIXME: We should check overlap here!
            auto appendAddress = ComputeEndAddress();
            activeSegment->CreateChunk(appendAddress);
            srcChunk->SetLoadAddress(appendAddress);
        } else {
            // Load address given, create chunk at this specific load address
            if (srcChunk->LoadAddress() < ComputeEndAddress()) {
                fmt::println(stderr, "Linker (in Context), chunk load address would overwrite existing data!");
                return;
            }
            activeSegment->CreateChunk(srcChunk->LoadAddress());
        }
        ReloacteChunkFromUnit(unit, srcChunk);
    }
}

// Relocates the active chunk from unit::<seg>::chunk
void Context::ReloacteChunkFromUnit(CompileUnit &unit, Segment::DataChunk::Ref srcChunk) {

    // Write data to it...
    Write(srcChunk->Data());
    // this is the current segment start address..
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
                // src chunk has been updated with the proper load address

                auto resolveAddress =  resolvePoint.placeholderAddress + resolvePoint.chunk->LoadAddress();
                auto targetSegment = GetSegment(resolvePoint.segment->Type());
                auto targetChunk = targetSegment->ChunkFromAddress(resolveAddress);
                if (targetChunk == nullptr) {
                    fmt::println("Linker (in Context), unable to resolve {} for resolve address {}", identifier->name,resolvePoint.placeholderAddress);
                    exit(1);
                }
                auto identAddress = identifier->relativeAddress + identifier->chunk->LoadAddress();
                targetChunk->ReplaceAt(resolveAddress, identAddress, resolvePoint.opSize);
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
    for(auto &[symbol, identifier] : unit.GetImports()) {
        auto exportedIdentifier = GetExport(symbol);
        if (exportedIdentifier == nullptr) {
            fmt::println(stderr, "Compiler, No such symbol {}", symbol);
            return;
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

            // the old 'import' resolve point is only used by the compile-unit
            // instead we compute it on a per unit per import and add it to the correct export
            // so when linking we only care about the full export table and all things it points to...

            IdentifierResolvePoint newResolvePoint = resolvePoint;
            newResolvePoint.placeholderAddress = newPlaceHolderAddress;
            exportedIdentifier->resolvePoints.push_back(newResolvePoint);
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
Segment::Ref Context::GetOrAddSegment(Segment::kSegmentType type) {
    auto seg = GetSegment(type);
    if (seg == nullptr) {
        seg = std::make_shared<Segment>(type);
        segments.push_back(seg);
    }
    activeSegment = seg;
    return seg;
}

//bool Context::CreateEmptySegment(const std::string &name) {
//    if (HaveSegment(name)) {
//        activeSegment = segments[name];
//        return true;
//    }
//
//    auto segment = std::make_shared<Segment>(name);
//    segments[name] = segment;
//    activeSegment = segment;
//    return true;
//}

//bool Context::SetActiveSegment(const std::string &name) {
//    if (!segments.contains(name)) {
//        return false;
//    }
//    activeSegment = segments.at(name);
//    if (activeSegment->currentChunk == nullptr) {
//        if (activeSegment->chunks.empty()) {
//            fmt::println(stderr, "Linker, Compiled unit can't link - no data!");
//            return false;
//        }
//        activeSegment->currentChunk =  activeSegment->chunks[0];
//    }
//    return true;
//}
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

bool Context::HaveSegment(Segment::kSegmentType type) {
    // easier to read..
    auto it = std::ranges::find_if(segments.begin(), segments.end(), [type](Segment::Ref v)-> bool {
        return (v->Type() == type);
    });
    return (it != segments.end());
}

const Segment::Ref Context::GetSegment(Segment::kSegmentType type) {
    auto it = std::ranges::find_if(segments.begin(), segments.end(), [type](Segment::Ref v)-> bool {
        return (v->Type() == type);
    });
    if (it == segments.end()) {
        return nullptr;
    }
    return *it;
}

size_t Context::GetSegments(std::vector<Segment::Ref> &outSegments) {
    outSegments.insert(outSegments.end(), segments.begin(), segments.end());
    return outSegments.size();
}
size_t Context::GetSegmentsOfType(Segment::kSegmentType ofType, std::vector<Segment::Ref> &outSegments) const {
    for (auto &unit : units) {
        for (auto &seg: unit.GetSegments()) {
            if (seg->Type() == ofType) {
                outSegments.push_back(seg);
            }
        }
    }
    // Sort them - this is what we normally want - and if we don't, it doesn't matter...
    std::sort(outSegments.begin(), outSegments.end(), [](const Segment::Ref &a, const Segment::Ref &b)->bool {
        return (b->StartAddress() - a->StartAddress());
    });

    return outSegments.size();
}

std::vector<Segment::Ref> &Context::GetSegments() {
    return segments;
}

const std::vector<Segment::Ref> &Context::GetSegments() const {
    return segments;
}



Segment::Ref Context::GetActiveSegment() {
    return activeSegment;
}


// FIXME: Remove this - unless used (in that case try to refactor)
size_t Context::GetSegmentEndAddress(Segment::kSegmentType type) {
    auto seg = GetSegment(type);
    if (seg == nullptr) {
        return 0;
    }
    return seg->EndAddress();
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
