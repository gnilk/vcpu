//
// Created by gnilk on 11.01.2024.
//

// FIXME: We are merging to the same context - we should merge into a local - new - context...   incoming context should be const!

#include "fmt/core.h"
#include "Segment.h"
#include "DummyLinker.h"
#include "InstructionSet.h"

using namespace gnilk;
using namespace gnilk::assembler;

static void VectorReplaceAt(std::vector<uint8_t> &data, uint64_t offset, int64_t newValue, vcpu::OperandSize opSize);


const std::vector<uint8_t> &DummyLinker::Data() {
    return linkedData;
}

// Link data in a context
bool DummyLinker::Link(Context &context) {

    // Now merge to one big blob...
    Merge(context);


    fmt::println("Relocate exported symbols");
    // Relocate symbols, this should all be done in the linker
    auto &data = context.Data();
    for(auto &[symbol, identifier] : context.GetExports()) {
        if (identifier->origIdentifier == nullptr) {
            fmt::println(stderr, "Linker, No such symbol '{}'", symbol);
            return false;
        }
        auto identAbsAddress = identifier->origIdentifier->absoluteAddress;
        fmt::println("  {} @ {:#x}", symbol, identAbsAddress);
        for(auto &resolvePoint : identifier->resolvePoints) {
            fmt::println("    - {:#x}",resolvePoint.placeholderAddress);
            if (!resolvePoint.isRelative) {
                VectorReplaceAt(data, resolvePoint.placeholderAddress, identAbsAddress, resolvePoint.opSize);
            } else {
                int64_t offset = static_cast<int64_t>(identAbsAddress) - static_cast<int64_t>(resolvePoint.placeholderAddress);
                if (offset < 0) {
                    offset -= 1;
                }
                VectorReplaceAt(data, resolvePoint.placeholderAddress, offset, resolvePoint.opSize);
            }
        }
    }



    return true;
}

// Merge stuff in the context
void DummyLinker::Merge(Context &context) {
    // Make sure this is empty...
    linkedData.clear();
    MergeAllSegments(context);
}

// Merge all segments
void DummyLinker::MergeAllSegments(Context &context) {
    size_t startAddress = 0;
    size_t endAddress = 0;

    size_t unitCounter = 0;

    // Merge segments across units and copy data...
    for(auto &unit : context.GetUnits()) {
        fmt::println("Merge segments in unit {}", unitCounter);
        unitCounter += 1;

        std::vector<Segment::Ref> unitSegments;
        unit.GetSegments(unitSegments);
        for(auto srcSeg : unitSegments) {
            // Make sure we have this segment
            context.GetOrAddSegment(srcSeg->Name());
            for(auto srcChunk : srcSeg->DataChunks()) {
                // Create out own chunk at the correct address - chunk merging happen later...
                if (srcChunk->IsLoadAddressAppend()) {
                    // No load-address given  - just append
                    // FIXME: We should check overlap here!
                    context.GetActiveSegment()->CreateChunk(context.GetActiveSegment()->EndAddress());

                } else {
                    // Load address given, create chunk at this specific load address
                    // FIXME: We should check overlap here!
                    context.GetActiveSegment()->CreateChunk(srcChunk->LoadAddress());
                }
                ReloacteChunkFromUnit(context, unit, srcChunk);
            }
        }
    }

    // Compute full binary end address...
    for(auto &[name, seg] : context.GetSegments()) {
        if (seg->EndAddress() > endAddress) {
            endAddress = seg->EndAddress();
        }
    }

    // Calculate the size of final binary imasge
    auto szFinalData = endAddress - startAddress;
    linkedData.resize(szFinalData);

    // Append the data
    for(auto &[name, seg] : context.GetSegments()) {
        for(auto &chunk : seg->DataChunks()) {
            memcpy(linkedData.data()+chunk->LoadAddress(), chunk->DataPtr(), chunk->Size());
        }
    }
}

// Relocates the active chunk from unit::<seg>::chunk
void DummyLinker::ReloacteChunkFromUnit(Context &context, const CompileUnit &unit, Segment::DataChunk::Ref srcChunk) {
    // Write data to it...
    Write(context, srcChunk->Data());

    // relocate local variables within this chunk...
    auto loadAddress = context.GetActiveSegment()->CurrentChunk()->LoadAddress();
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
                context.GetActiveSegment()->CurrentChunk()->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, identifier->absoluteAddress, resolvePoint.opSize);
            } else {
                int64_t offset = static_cast<int64_t>(identifier->absoluteAddress) - static_cast<int64_t>(resolvePoint.placeholderAddress);
                if (offset < 0) {
                    offset -= 1;
                }
                context.GetActiveSegment()->CurrentChunk()->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, offset, resolvePoint.opSize);
            }
        }
    }

    // Update resolve points for any public identifier (export) belonging to this chunk
    // Since the chunk might have moved - the resolvePoint (i.e. the point to be modified when resolving identifiers) might have moved
    // So we need to adjust all of them...
    for(auto &[symbol, identifier] : unit.GetImports()) {
        auto exportedIdentifier = context.GetExport(symbol);
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
            auto dstLoad = context.GetActiveSegment()->CurrentChunk()->LoadAddress();
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

size_t DummyLinker::Write(Context &context, const std::vector<uint8_t> &data) {
    if (!EnsureChunk(context)) {
        return 0;
    }
    return context.GetActiveSegment()->CurrentChunk()->Write(data);
}

bool DummyLinker::EnsureChunk(Context &context) {
    if (context.GetActiveSegment() == nullptr) {
        fmt::println(stderr, "Compiler, need at least an active segment");
        return false;
    }
    if (context.GetActiveSegment()->CurrentChunk() != nullptr) {
        return true;
    }
    context.GetActiveSegment()->CreateChunk(context.GetCurrentWriteAddress());
    return true;
}




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
