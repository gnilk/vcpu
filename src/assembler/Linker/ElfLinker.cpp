//
// Created by gnilk on 11.01.2024.
//

#include "elfio/elfio.hpp"
#include "fmt/core.h"
#include <sstream>
#include "ElfLinker.h"

#include "MemorySubSys/MemoryUnit.h"

using namespace ELFIO;
using namespace gnilk::assembler;

const std::vector<uint8_t> &ElfLinker::Data() {
    return elfData;
}


bool ElfLinker::Link(gnilk::assembler::Context &context) {
    if (!Merge(context)) {
        return false;
    }

    if (!WriteElf()) {
        return false;
    }
    auto f = fopen("dummy.bin", "w+");
    if (f == nullptr) {
        return false;
    }
    fwrite(elfData.data(), elfData.size(), 1, f);
    fclose(f);
    return true;
}

// I could use the elf to relocate my binary - but it has already been done so I don't quite need it..
// And - there is a problem - my relocation table points to the source-chunks and not the merged chunks..
bool ElfLinker::WriteElf() {
    elfWriter.create(ELFCLASS64, ELFDATA2LSB );

    elfWriter.set_os_abi(ELFOSABI_NONE );
    elfWriter.set_type(ET_EXEC );
    elfWriter.set_machine(EM_68K );

    for(auto &seg : destContext.GetSegments()) {
        auto elfSeg = elfWriter.segments.add();
        elfSeg->set_virtual_address(seg->StartAddress());
        elfSeg->set_physical_address(seg->StartAddress());
        elfSeg->set_flags(PF_X | PF_R);
        elfSeg->set_align(vcpu::VCPU_MMU_PAGE_SIZE);

        for(auto chunk : seg->DataChunks()) {
            auto elfSection = elfWriter.sections.add("noname");

            elfSection->set_address(chunk->LoadAddress());
            elfSection->set_type(SHT_PROGBITS);
            elfSection->set_flags( SHF_ALLOC | SHF_EXECINSTR );
            // WHY do you write a library accepting 'char *' and not 'void *' here???
            elfSection->append_data((const char *)chunk->DataPtr(), chunk->Size());

        }
    }



    // Not sure if this is the right method...
    if (destContext.HasStartAddress()) {
        elfWriter.set_entry(destContext.GetStartAddress()->absoluteAddress);
    } else {
        fmt::println(stderr, "ElfLinker, no entry point defined, setting to 0 (zero)");
        elfWriter.set_entry(0);
    }

    // Save this to a 'stringstream' (CPP Streams are: @#$!@$!@#)
    std::ostringstream outStream(std::ios_base::binary | std::ios_base::out);
    elfWriter.save(outStream);
    outStream.flush();

    // Convert the 'string' to a vector of bytes...
    // This can be an array - but that's a refactoring for another day...
    auto strData = outStream.rdbuf()->str();
    auto ptrData = strData.data();
    auto szData = strData.size();

    elfData.insert(elfData.end(), ptrData, ptrData + szData);
    return true;

}

bool ElfLinker::WriteElfOld(CompileUnit &unit) {
    // You can't proceed without this function call!
    elfWriter.create(ELFCLASS64, ELFDATA2LSB );

    elfWriter.set_os_abi(ELFOSABI_NONE );
    elfWriter.set_type(ET_EXEC );
    elfWriter.set_machine(EM_68K );

    auto textSeg = unit.GetSegment(Segment::kSegmentType::Code);
    if (textSeg == nullptr) {
        return false;
    }


    auto elfSeg = elfWriter.segments.add();

    for(auto &seg : destContext.GetSegments()) {
        auto elfSeg = elfWriter.segments.add();
        elfSeg->set_virtual_address(textSeg->StartAddress());
        elfSeg->set_physical_address(textSeg->StartAddress());
        elfSeg->set_flags(PF_X | PF_R);
        elfSeg->set_align(vcpu::VCPU_MMU_PAGE_SIZE);


        for(auto chunk : seg->DataChunks()) {
            auto elfSection = elfWriter.sections.add("noname");

            elfSection->set_address(chunk->LoadAddress());
            elfSection->set_type(SHT_PROGBITS);
            elfSection->set_flags( SHF_ALLOC | SHF_EXECINSTR );
            // WHY do you write a library accepting 'char *' and not 'void *' here???
            elfSection->append_data((const char *)chunk->DataPtr(), chunk->Size());

            elfSeg->add_section(elfSection, 0x10);
        }
    }
    section *elfTextSec = elfWriter.sections.add(".text");
    elfTextSec->set_type( SHT_PROGBITS );
    elfTextSec->set_flags( SHF_ALLOC | SHF_EXECINSTR );
    elfTextSec->set_addr_align( 0x10 );
    // FIXME...
    //elfTextSec->set_data((const char *)textSeg->DataPtr(), textSeg->Size());

    segment *elfTextSeg = elfWriter.segments.add();
    elfTextSeg->set_type(PT_LOAD);
    // FIXME:
    elfTextSeg->set_virtual_address(textSeg->StartAddress());
    elfTextSeg->set_physical_address(textSeg->StartAddress());
    elfTextSeg->set_flags(PF_X | PF_R);
    elfTextSeg->set_align(vcpu::VCPU_MMU_PAGE_SIZE);
    elfTextSeg->add_section(elfTextSec, elfTextSec->get_addr_align());

    auto dataSeg = unit.GetSegment(Segment::kSegmentType::Data);
    // FIXME: this is not an error...
    if (dataSeg == nullptr) {
        fmt::println(stderr,"ElfLinker, no data segment!");
        return false;
    }
    section *elfDataSec = elfWriter.sections.add(".data");
    elfDataSec->set_type(SHT_PROGBITS);
    elfDataSec->set_flags(SHF_ALLOC | SHF_WRITE);
    elfDataSec->set_addr_align(0x04);   // verify!
    elfDataSec->set_data((const char *)dataSeg->CurrentChunk()->DataPtr(), dataSeg->CurrentChunk()->Size());

    segment *elfDataSeg = elfWriter.segments.add();
    elfDataSeg->set_type(PT_LOAD);
    elfDataSeg->set_virtual_address(dataSeg->StartAddress());
    elfDataSeg->set_physical_address(dataSeg->StartAddress());
    elfDataSeg->set_flags(PF_W | PF_R);
    elfDataSeg->set_align(vcpu::VCPU_MMU_PAGE_SIZE);
    elfDataSeg->add_section(elfDataSec, elfDataSec->get_addr_align());

    elfWriter.set_entry(elfTextSeg->get_virtual_address());

    // Save this to a 'stringstream' (CPP Streams are: @#$!@$!@#)
    std::ostringstream outStream(std::ios_base::binary | std::ios_base::out);
    elfWriter.save(outStream);
    outStream.flush();

    // Convert the 'string' to a vector of bytes...
    // This can be an array - but that's a refactoring for another day...
    auto strData = outStream.rdbuf()->str();
    auto ptrData = strData.data();
    auto szData = strData.size();

    elfData.insert(elfData.end(), ptrData, ptrData + szData);
    return true;
}


/////////
//
// This is the old linker code - before all refactoring took place, keeping it here until this one has been updated
// as it actually worked
//
/*
bool ElfLinker::LinkOld(CompiledUnit &unit, std::unordered_map<std::string, Identifier> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders) {
    std::vector<Segment::Ref> segments;
    unit.GetSegments(segments);

    auto textSeg = unit.GetSegment(".text");
    if (textSeg == nullptr) {
        fmt::println(stderr, "Linker, no '.text' segment found - i.e. no code");
        return false;
    }

    auto dataSeg = unit.GetSegment(".data");
    if (dataSeg == nullptr) {
        fmt::println(stderr, "Linker, no data segment?");
    } else {
        auto ofsDataSeg = textSeg->EndAddress();
        // put the data segment at the next MMU page after the text segment
        ofsDataSeg = (ofsDataSeg & (~(vcpu::VCPU_MMU_PAGE_SIZE - 1)));
        ofsDataSeg +=  + vcpu::VCPU_MMU_PAGE_SIZE;
        dataSeg->SetLoadAddress(ofsDataSeg);
    }

    fmt::println("Segments:");
    for(auto &s : segments) {
        fmt::println("  Name={}  LoadAddress={:#x}  EndAddress={:#x}",s->Name(), s->StartAddress(), s->EndAddress());
    }

    fmt::println("Linking");
    if (!RelocateIdentifiers(unit, identifierAddresses, addressPlaceholders)) {
        return false;
    }

    WriteElf(unit);
    return true;
}

bool ElfLinker::RelocateIdentifiers(CompiledUnit &unit, std::unordered_map<std::string, Identifier> &identifierAddresses, std::vector<IdentifierAddressPlaceholder::Ref> &addressPlaceholders) {
    for(auto &placeHolder : addressPlaceholders) {
        if (!identifierAddresses.contains(placeHolder->ident)) {
            fmt::println(stderr, "Unknown identifier: {}", placeHolder->ident);
            return false;
        }
        // This address is in a specific segment, we need to store that as well for the placeholder
        auto identifierAddr = identifierAddresses[placeHolder->ident];
        if (placeHolder->isRelative) {
            RelocateRelative(unit, identifierAddr, placeHolder);
        } else {
            RelocateAbsolute(unit, identifierAddr, placeHolder);
        }

    }
    return true;
}

bool ElfLinker::RelocateRelative(CompiledUnit &unit, Identifier &identifierAddr, IdentifierAddressPlaceholder::Ref &placeHolder) {
    if (placeHolder->segment != identifierAddr.segment) {
        fmt::println(stderr, "Relative addressing only within same segment! - check: {}", placeHolder->ident);
        return false;
    }
    fmt::println("  from:{}, to:{}", placeHolder->ofsRelative, identifierAddr.absoluteAddress);

    auto placeHolderChunk = placeHolder->segment->ChunkFromAddress(placeHolder->address);
    auto identChunk = identifierAddr.segment->ChunkFromAddress(identifierAddr.absoluteAddress);

    fmt::println("  REL: {}@{:#x} = {}@{:#x}",
        placeHolder->ident, placeHolderChunk->LoadAddress() + placeHolder->address,
        identifierAddr.segment->Name(),identChunk->LoadAddress() + identifierAddr.absoluteAddress);

    int offset = 0;
    if (identifierAddr.absoluteAddress > placeHolder->ofsRelative) {
        // FIXME: This is a bit odd - but if we jump forward I need to add 1 to the offset..
        offset = identifierAddr.absoluteAddress - placeHolder->ofsRelative + 1;
    } else {
        offset = identifierAddr.absoluteAddress - placeHolder->ofsRelative;
    }

    placeHolder->segment->ReplaceAt(placeHolder->address, offset, placeHolder->opSize);

    return true;
}

bool ElfLinker::RelocateAbsolute(CompiledUnit &unit, Identifier &identifierAddr, IdentifierAddressPlaceholder::Ref &placeHolder) {
    auto placeHolderChunk = placeHolder->segment->ChunkFromAddress(placeHolder->address);
    auto identChunk = identifierAddr.segment->ChunkFromAddress(identifierAddr.absoluteAddress);


    fmt::println("  {}@{:#x} = {}@{:#x}",
        placeHolder->ident, placeHolderChunk->LoadAddress() + placeHolder->address,
        identifierAddr.segment->Name(),identifierAddr.absoluteAddress);

    // WEFU@#$T)&#$YTG(# - DO NOT ASSUME we only want to 'lea' from .data!!!!
    if (identifierAddr.segment == nullptr) {
        fmt::println(stderr, "Linker: no segment for identifier '{}'", placeHolder->ident);
        return false;
    }
    if (placeHolder->segment == nullptr) {
        fmt::println(stderr, "Linker, placehold has no segment - can't replace!");
        return false;
    }

    placeHolder->segment->ReplaceAt(placeHolder->address, identifierAddr.absoluteAddress);

    return true;
}
*/
