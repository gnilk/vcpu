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
bool DummyLinker::Link(const Context &context) {

    // Now merge to one big blob...
    if (!Merge(context)) {
        return false;
    }


    fmt::println("Relocate exported symbols");


    // Relocate symbols, this should all be done in the linker
    auto &data = destContext.Data();
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
bool DummyLinker::Merge(const Context &context) {
    // Make sure this is empty...
    linkedData.clear();
    return MergeAllSegments(context);
}

// Merge all segments
bool DummyLinker::MergeAllSegments(const Context &sourceContext) {
    size_t startAddress = 0;
    size_t endAddress = 0;

    size_t unitCounter = 0;

    // Merge segments across units and copy data...
    for(auto &unit : sourceContext.GetUnits()) {
        fmt::println("Merge segments in unit {}", unitCounter);
        unitCounter += 1;

        std::vector<Segment::Ref> unitSegments;
        unit.GetSegments(unitSegments);
        for(auto srcSeg : unitSegments) {
            // Make sure we have this segment
            destContext.GetOrAddSegment(srcSeg->Name());
            for(auto srcChunk : srcSeg->DataChunks()) {
                // Create out own chunk at the correct address - chunk merging happen later...
                if (srcChunk->IsLoadAddressAppend()) {
                    // No load-address given  - just append
                    // FIXME: We should check overlap here!
                    destContext.GetActiveSegment()->CreateChunk(destContext.GetActiveSegment()->EndAddress());

                } else {
                    // Load address given, create chunk at this specific load address
                    // FIXME: We should check overlap here!
                    destContext.GetActiveSegment()->CreateChunk(srcChunk->LoadAddress());
                }

                if (!ReloacteChunkFromUnit(sourceContext, unit, srcChunk)) {
                    return false;
                }
            }
        }
    }

    // Compute full binary end address...
    for(auto &[name, seg] : destContext.GetSegments()) {
        if (seg->EndAddress() > endAddress) {
            endAddress = seg->EndAddress();
        }
    }

    // Calculate the size of final binary imasge
    auto szFinalData = endAddress - startAddress;
    linkedData.resize(szFinalData);

    // Append the data
    for(auto &[name, seg] : destContext.GetSegments()) {
        for(auto &chunk : seg->DataChunks()) {
            memcpy(linkedData.data()+chunk->LoadAddress(), chunk->DataPtr(), chunk->Size());
        }
    }

    return true;
}

// Relocates the active chunk from unit::<seg>::chunk
bool DummyLinker::ReloacteChunkFromUnit(const Context &sourceContext, const CompileUnit &unit, Segment::DataChunk::Ref srcChunk) {
    // Write data to it...
    Write( srcChunk->Data());

    // relocate local variables within this chunk...
    auto loadAddress = destContext.GetActiveSegment()->CurrentChunk()->LoadAddress();
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
                destContext.GetActiveSegment()->CurrentChunk()->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, identifier->absoluteAddress, resolvePoint.opSize);
            } else {
                int64_t offset = static_cast<int64_t>(identifier->absoluteAddress) - static_cast<int64_t>(resolvePoint.placeholderAddress);
                if (offset < 0) {
                    offset -= 1;
                }
                destContext.GetActiveSegment()->CurrentChunk()->ReplaceAt(resolvePoint.placeholderAddress + loadAddress, offset, resolvePoint.opSize);
            }
        }
    }

    // FIXME: we change the sourceContext here - this is wrong...

    // Update resolve points for any public identifier (export) belonging to this chunk
    // Since the chunk might have moved - the resolvePoint (i.e. the point to be modified when resolving identifiers) might have moved
    // So we need to adjust all of them...
    for(auto &[symbol, identifier] : unit.GetImports()) {
        auto exportedIdentifier = sourceContext.GetExport(symbol);
        if (exportedIdentifier == nullptr) {
            fmt::println(stderr, "Compiler, No such symbol {}", symbol);
            return false;
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

            // FIXME: this is wrong...
            auto dstLoad = destContext.GetActiveSegment()->CurrentChunk()->LoadAddress();
            auto delta = static_cast<int64_t>(dstLoad) - static_cast<int64_t>(srcLoad);
            // Compute the new placeholder address...
            auto newPlaceHolderAddress = resolvePoint.placeholderAddress + delta;
            fmt::println("  srcLoad={}, dstLoad={}, delta={}, oldPlacedHolder={}, newPlaceHolder={}", srcLoad, dstLoad, delta, resolvePoint.placeholderAddress, newPlaceHolderAddress);

            // the old 'import' resolve point is only used by the compile-unit
            // instead we compute it on a per unit per import and add it to the correct export
            // so when linking we only care about the full export table and all things it points to...

            // FIXME: These should be placed locally...
            IdentifierResolvePoint newResolvePoint = resolvePoint;
            newResolvePoint.placeholderAddress = newPlaceHolderAddress;
            exportedIdentifier->resolvePoints.push_back(newResolvePoint);
        }
    }
    return true;
}

size_t DummyLinker::Write(const std::vector<uint8_t> &data) {
    if (!EnsureChunk()) {
        return 0;
    }
    return destContext.GetActiveSegment()->CurrentChunk()->Write(data);
}

bool DummyLinker::EnsureChunk() {
    if (destContext.GetActiveSegment() == nullptr) {
        fmt::println(stderr, "Compiler, need at least an active segment");
        return false;
    }
    if (destContext.GetActiveSegment()->CurrentChunk() != nullptr) {
        return true;
    }
    destContext.GetActiveSegment()->CreateChunk(destContext.GetCurrentWriteAddress());
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
