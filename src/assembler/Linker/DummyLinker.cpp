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

bool DummyLinker::Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders) {
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

    auto baseAddress = unit.GetBaseAddress();
    auto dataSeg = unit.GetSegment(".data");
    if (dataSeg != nullptr) {
        dataSeg->SetLoadAddress(unit.GetCurrentWritePtr());
    }

    // Since we merge the segments we need to replace this...
    if (unit.HaveSegment(".data")) {
        unit.GetSegment(".data")->SetLoadAddress(ofsDataSegStart + baseAddress);
    }

    //
    // Link code to stuff
    //

    fmt::println("Linking");
    for(auto &placeHolder : addressPlaceholders) {
        if (!identifierAddresses.contains(placeHolder.ident)) {
            fmt::println(stderr, "Unknown identifier: {}", placeHolder.ident);
            return false;
        }
        // This address is in a specific segment, we need to store that as well for the placeholder
        auto identifierAddr = identifierAddresses[placeHolder.ident];

        if (placeHolder.isRelative) {
            if (placeHolder.segment != identifierAddr.segment) {
                fmt::println(stderr, "Relative addressing only within same segment! - check: {}", placeHolder.ident);
                return false;
            }
            fmt::println("  from:{}, to:{}", placeHolder.ofsRelative, identifierAddr.address);

//            fmt::println("  REL: {}@{:#x} = {}@{:#x}",
//                placeHolder.ident, placeHolder.address - placeHolder.segment->LoadAddress(),
//                identifierAddr.segment->Name(),identifierAddr.segment->LoadAddress() + identifierAddr.address);

            auto placeHolderChunk = placeHolder.segment->ChunkFromAddress(placeHolder.address);
            auto identChunk = identifierAddr.segment->ChunkFromAddress(identifierAddr.address);

            fmt::println("  REL: {}@{:#x} = {}@{:#x}",
                placeHolder.ident, placeHolder.address - placeHolderChunk->LoadAddress(),
                identifierAddr.segment->Name(),identChunk->LoadAddress() + identifierAddr.address);


            int offset = 0;
            if (identifierAddr.address > placeHolder.ofsRelative) {
                // FIXME: This is a bit odd - but if we jump forward I need to add 1 to the offset..
                offset = identifierAddr.address - placeHolder.ofsRelative + 1;
            } else {
                offset = identifierAddr.address - placeHolder.ofsRelative;
            }
            //unit.ReplaceAt(placeHolder.address - placeHolder.segment->LoadAddress(), offset, placeHolder.opSize);
            unit.ReplaceAt(placeHolder.address - placeHolderChunk->LoadAddress(), offset, placeHolder.opSize);
        } else {
            auto placeHolderChunk = placeHolder.segment->ChunkFromAddress(placeHolder.address);
            auto identChunk = identifierAddr.segment->ChunkFromAddress(identifierAddr.address);

            fmt::println("  {}@{:#x} = {}@{:#x}",
                placeHolder.ident, placeHolder.address - placeHolderChunk->LoadAddress(),
                identifierAddr.segment->Name(),identChunk->LoadAddress() + identifierAddr.address);

            //            fmt::println("  {}@{:#x} = {}@{:#x}",
//                placeHolder.ident, placeHolder.address - placeHolder.segment->LoadAddress(),
//                identifierAddr.segment->Name(),identifierAddr.segment->LoadAddress() + identifierAddr.address);


            // WEFU@#$T)&#$YTG(# - DO NOT ASSUME we only want to 'lea' from .data!!!!
            if (identifierAddr.segment == nullptr) {
                fmt::println(stderr, "Linker: no segment for identifier '{}'", placeHolder.ident);
                exit(1);
            }

            //unit.ReplaceAt(placeHolder.address - placeHolder.segment->LoadAddress(), identifierAddr.segment->LoadAddress() + identifierAddr.address);
            unit.ReplaceAt(placeHolder.address - placeHolderChunk->LoadAddress(), identChunk->LoadAddress() + identifierAddr.address);
        }
    }
    auto linkOutputSegment = unit.GetSegment("link_out");
    // FIXME: THIS IS WRONG!@!#!@$!@$
    for(auto chunk : linkOutputSegment->DataChunks()) {
        auto data = chunk->Data();
        linkedData.insert(linkedData.begin(), data.begin(), data.end());

    }
    return true;

}