//
// Created by gnilk on 11.01.2024.
//

#include "fmt/core.h"
#include "Segment.h"
#include "DummyLinker.h"

using namespace gnilk::assembler;
const std::vector<uint8_t> &DummyLinker::Data() {
    return linkedData;
}

bool DummyLinker::Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders) {
    std::vector<Segment::Ref> segments;
    unit.GetSegments(segments);
    fmt::println("Segments:");
    for(auto &s : segments) {
        fmt::println("  Name={}",s->Name());
        for(auto &chunk : s->DataChunks()) {
            fmt::println("   LoadAddress={}", chunk->LoadAddress());
        }
    }

    // Merge segments (.text and .data) to hidden 'link_out' segment...
    unit.MergeSegments("link_out", ".text");
    size_t ofsDataSegStart = unit.GetSegmentEndAddress("link_out");

    // This will just fail if '.data' doesnt' exists..
    if (!unit.MergeSegments("link_out", ".data")) {
        fmt::println("No data segment, merging directly into '.text'");
        // No data segment, well - we reset this one in that case...
        ofsDataSegStart = 0;
    }


    // This is our target now...
    unit.SetActiveSegment("link_out");

    // FIXME: Trying to relocate the data segment - I think - but this is horribly wrong...
    auto baseAddress = unit.GetBaseAddress();
    auto dataSeg = unit.GetSegment(".data");
    if (dataSeg != nullptr) {
        dataSeg->SetLoadAddress(unit.GetCurrentWriteAddress());
    }

    // Since we merge the segments we need to replace this...
    if (unit.HaveSegment(".data")) {
        unit.GetSegment(".data")->SetLoadAddress(ofsDataSegStart + baseAddress);
    }

    //
    // FIXME: REWRITE THIS!
    // Perform relocation...
    //
    fmt::println("Linking");
    for(auto &placeHolder : addressPlaceholders) {
        if (!identifierAddresses.contains(placeHolder->ident)) {
            fmt::println(stderr, "Unknown identifier: {}", placeHolder->ident);
            return false;
        }
        // This address is in a specific segment, we need to store that as well for the placeholder
        auto identifierAddr = identifierAddresses[placeHolder->ident];

        if (placeHolder->isRelative) {
            if (placeHolder->segment != identifierAddr.segment) {
                fmt::println(stderr, "Relative addressing only within same segment! - check: {}", placeHolder->ident);
                return false;
            }
            fmt::println("  from:{}, to:{}", placeHolder->ofsRelative, identifierAddr.absoluteAddress);

//            fmt::println("  REL: {}@{:#x} = {}@{:#x}",
//                placeHolder.ident, placeHolder.address - placeHolder.segment->LoadAddress(),
//                identifierAddr.segment->Name(),identifierAddr.segment->LoadAddress() + identifierAddr.address);

            auto placeHolderChunk = placeHolder->chunk; //placeHolder.segment->ChunkFromAddress(placeHolder.address);
            auto identChunk = identifierAddr.chunk; //identifierAddr.segment->ChunkFromAddress(identifierAddr.address);

            fmt::println("  REL: {}@{:#x} = {}@{:#x}",
                placeHolder->ident, placeHolder->address + placeHolderChunk->LoadAddress(),
                identifierAddr.segment->Name(),identChunk->LoadAddress() + identifierAddr.absoluteAddress);


            int offset = 0;
            if (identifierAddr.absoluteAddress > placeHolder->ofsRelative) {
                // FIXME: This is a bit odd - but if we jump forward I need to add 1 to the offset..
                offset = identifierAddr.absoluteAddress - placeHolder->ofsRelative + 1;
            } else {
                offset = identifierAddr.absoluteAddress - placeHolder->ofsRelative;
            }
            //unit.ReplaceAt(placeHolder.address - placeHolder.segment->LoadAddress(), offset, placeHolder.opSize);
            //unit.ReplaceAt(placeHolder.address - placeHolderChunk->LoadAddress(), offset, placeHolder.opSize);
            placeHolderChunk->ReplaceAt(placeHolder->address, offset, placeHolder->opSize);
        } else {

            // FIXME: Verify segment pointers before doing this!

            auto placeHolderChunk = placeHolder->chunk; //placeHolder.segment->ChunkFromAddress(placeHolder.address);
            // I have already relocated this - segment, which is bad...
            auto identChunk = identifierAddr.chunk; //identifierAddr.segment->ChunkFromAddress(identifierAddr.address + identifierAddr.segment->StartAddress());

            if ((placeHolderChunk==nullptr) || (identChunk == nullptr)) {
                if (placeHolderChunk == nullptr) {
                    fmt::println(stderr, "Linker, Can't find Segment::Chunk for '{}'", placeHolder->ident);
                    return false;
                }

                fmt::println(stderr, "Linker, Identifier '{}' @ {} has no valid Segment::Chunk", placeHolder->ident, identifierAddr.absoluteAddress);
                return false;
            }

            fmt::println("  {}@{:#x} = {}@{:#x}",
                placeHolder->ident, placeHolder->address + placeHolderChunk->LoadAddress(),
                identifierAddr.segment->Name(),identChunk->LoadAddress() + identifierAddr.absoluteAddress);


            placeHolderChunk->ReplaceAt(placeHolder->address, identifierAddr.absoluteAddress + identifierAddr.segment->StartAddress());

            // WEFU@#$T)&#$YTG(# - DO NOT ASSUME we only want to 'lea' from .data!!!!
            if (identifierAddr.segment == nullptr) {
                fmt::println(stderr, "Linker: no segment for identifier '{}'", placeHolder->ident);
                exit(1);
            }

            //unit.ReplaceAt(placeHolder.address - placeHolder.segment->LoadAddress(), identifierAddr.segment->LoadAddress() + identifierAddr.address);
            //unit.ReplaceAt(placeHolder.address - placeHolderChunk->LoadAddress(), identChunk->LoadAddress() + identifierAddr.address);
        }
    }
    auto linkOutputSegment = unit.GetSegment("link_out");
    // FIXME: THIS IS WRONG!@!#!@$!@$
    // Compute total size...

    auto size = linkOutputSegment->EndAddress();
    linkedData.resize(size);
    for(auto chunk : linkOutputSegment->DataChunks()) {
        auto data = chunk->Data();
        auto endOfs = chunk->LoadAddress() + data.size();
        if (endOfs > linkedData.capacity()) {
            // We have a problem..
            fmt::println(stderr, "Linker, final link output data would exceed buffer - size computation mismatch - critical!");
            return false;
        }
        // I really wish there was a way to actually do this through the interface...
        // But I couldn't find any...
        memcpy(linkedData.data()+chunk->LoadAddress(), data.data(), data.size());
    }
    return true;

}