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

#ifndef GLB_IDENT_START_POINT
#define GLB_IDENT_START_POINT "_start"
#endif

static const std::string glbStartPointSymbol(GLB_IDENT_START_POINT);

void Context::Clear() {
    segments.clear();
    units.clear();
    activeSegment = nullptr;
    startAddressIdentifier = nullptr;
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

bool Context::Merge() {
    // Make sure this is empty...
    outputData.clear();
    return MergeUnits(outputData);
}

bool Context::MergeUnits(std::vector<uint8_t> &out) {
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

    // Relocate exports - this happens within the chunks, which are still present
    if (!RelocateExports()) {
        return false;
    }

    // NOTE: ElfWriter does not need this
    // FIXME: Make this an explicit step..
    FinalMergeOfSegments(out);

    return true;
}

// Merge all segments within a single compile unit...
// Segments are merged in declaration order...
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

            // Create a chunk in our own context segment
            activeSegment->CreateChunk(appendAddress);
            srcChunk->SetLoadAddress(appendAddress);
        } else {
            // Load address given, create chunk at this specific load address - if possible...
            if (srcChunk->LoadAddress() < ComputeEndAddress()) {
                fmt::println(stderr, "Linker (in Context), chunk load address would overwrite existing data!");
                return;
            }
            activeSegment->CreateChunk(srcChunk->LoadAddress());
        }
        RelocateChunkFromUnit(unit, srcChunk);
    }
}

void Context::FinalMergeOfSegments(std::vector<uint8_t> &out) {
    // This can only be done after merging...
    auto endAddress = ComputeEndAddress();
    auto startAddress = 0;      // This is the FIRMWARE start address for the memory range (not the actual address of '_start')


    if (startAddressIdentifier != nullptr) {
        auto codeStartAddress = startAddressIdentifier->absoluteAddress;
        // Note: we don't store this - we just detect it and the ELF or executable writer has to take care...
    }

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


uint64_t Context::ComputeEndAddress() {
    uint64_t endAddress = 0;
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


// Relocates the active chunk from unit::<seg>::chunk
bool Context::RelocateChunkFromUnit(CompileUnit &unit, Segment::DataChunk::Ref srcChunk) {

    // Write data to it...
    Write(srcChunk->Data());
    // this is the current segment start address..
    auto loadAddress = activeSegment->currentChunk->LoadAddress();

    for (auto &[symbol, identifier] : unit.GetIdentifiers()) {
        // Relocate all within this chunk...
        if (identifier->chunk != srcChunk) {
            continue;
        }

        if (identifier->name == glbStartPointSymbol) {
            if (HasStartAddress()) {
                fmt::println("Compiler, '{}' defined twice. '{}' is a reserved symbol defining the entry point for the binary", glbStartPointSymbol, glbStartPointSymbol);
                return false;
            }
            SetStartAddress(identifier);
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
                if (!targetChunk->ReplaceAt(resolveAddress, identAddress, resolvePoint.opSize)) {
                    return false;
                }
            } else {
                int64_t offset = static_cast<int64_t>(identifier->absoluteAddress) - static_cast<int64_t>(resolvePoint.placeholderAddress);
                if (offset < 0) {
                    offset -= 1;
                }
                if (!activeSegment->currentChunk->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, offset, resolvePoint.opSize)) {
                    return false;
                }
            }
        }
    } // for (auto &[symbol, identifier] : unit.GetIdentifiers())

    // Update resolve points for any public identifier (export) belonging to this chunk
    // Since the chunk might have moved - the resolvePoint (i.e. the point to be modified when resolving identifiers) might have moved
    // So we need to adjust all of them...
    for(auto &[symbol, identifier] : unit.GetImports()) {
        auto exportedIdentifier = GetExport(symbol);
        if (exportedIdentifier == nullptr) {
            fmt::println(stderr, "Compiler, No such symbol {}", symbol);
            return false;
        }

        // Note: This must be done before we check resolve points as the resolve-point array _should_ be empty for start
        //       But it is nothing we prohibit...
        // If this is the globally recognized starting point - set the start address for the binary
        // Unless defined elsewhere...

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

    return true;
}

bool Context::RelocateExports() {
    fmt::println("Relocate exported symbols");

    // Relocate symbols, this should all be done in the linker
    for(auto &[symbol, identifier] : GetExports()) {
        if (identifier->origIdentifier == nullptr) {
            fmt::println(stderr, "Linker, No such symbol '{}'", symbol);
            return false;
        }
        //auto identAbsAddress = identifier->origIdentifier->absoluteAddress;
        auto identAbsAddress = identifier->origIdentifier->relativeAddress + identifier->origIdentifier->chunk->LoadAddress();
        fmt::println("  {} @ {:#x}", symbol, identAbsAddress);
        for(auto &resolvePoint : identifier->resolvePoints) {
            if (resolvePoint.isRelative) {
                fmt::println(stderr, "Linker, Relative addressing not possible between compile units!");
                fmt::println(stderr, "        Identifier={}", identifier->name);
                exit(1);
            }

            auto resolveAddress =  resolvePoint.placeholderAddress + resolvePoint.chunk->LoadAddress();
            auto targetSegment = GetSegment(resolvePoint.segment->Type());
            auto targetChunk = targetSegment->ChunkFromAddress(resolveAddress);
            if (targetChunk == nullptr) {
                fmt::println("Linker (in Context), unable to resolve {} for resolve address {}", identifier->name,resolvePoint.placeholderAddress);
                exit(1);
            }
            //auto identAddress = identifier->origIdentifier->absoluteAddress + identifier->origIdentifier->chunk->LoadAddress();

            fmt::println("    - {:#x}",resolveAddress);
            if (!targetChunk->ReplaceAt(resolvePoint.placeholderAddress, identAbsAddress, resolvePoint.opSize)) {
                return false;
            }
        }
    }
    return true;
}

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

std::vector<Segment::Ref> &Context::GetSegments() {
    return segments;
}

Segment::Ref Context::GetActiveSegment() {
    return activeSegment;
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
