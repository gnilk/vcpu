//
// Created by gnilk on 11.01.2024.
//

#include "elfio/elfio.hpp"
#include "fmt/core.h"
#include <sstream>
#include "ElfLinker.h"

#include "MemoryUnit.h"

using namespace ELFIO;
using namespace gnilk::assembler;

const std::vector<uint8_t> &ElfLinker::Data() {
    return elfData;
}



bool ElfLinker::Link(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders) {
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
        auto ofsDataSeg = textSeg->LoadAddress() + textSeg->Size();
        // put the data segment at the next MMU page after the text segment
        ofsDataSeg = (ofsDataSeg & (~(vcpu::VCPU_MMU_PAGE_SIZE - 1)));
        ofsDataSeg +=  + vcpu::VCPU_MMU_PAGE_SIZE;
        dataSeg->SetLoadAddress(ofsDataSeg);
    }

    fmt::println("Segments:");
    for(auto &s : segments) {
        fmt::println("  Name={}  LoadAddress={:#x}  Size={:#x}",s->Name(), s->LoadAddress(), s->Size());
    }

    fmt::println("Linking");
    if (!RelocateIdentifiers(unit, identifierAddresses, addressPlaceholders)) {
        return false;
    }

    WriteElf(unit);
    return true;
}

bool ElfLinker::RelocateIdentifiers(CompiledUnit &unit, std::unordered_map<std::string, IdentifierAddress> &identifierAddresses, std::vector<IdentifierAddressPlaceholder> &addressPlaceholders) {
    for(auto &placeHolder : addressPlaceholders) {
        if (!identifierAddresses.contains(placeHolder.ident)) {
            fmt::println(stderr, "Unknown identifier: {}", placeHolder.ident);
            return false;
        }
        // This address is in a specific segment, we need to store that as well for the placeholder
        auto identifierAddr = identifierAddresses[placeHolder.ident];
        if (placeHolder.isRelative) {
            RelocateRelative(unit, identifierAddr, placeHolder);
        } else {
            RelocateAbsolute(unit, identifierAddr, placeHolder);
        }

    }
    return true;
}

bool ElfLinker::RelocateRelative(CompiledUnit &unit, IdentifierAddress &identifierAddr, IdentifierAddressPlaceholder &placeHolder) {
    if (placeHolder.segment != identifierAddr.segment) {
        fmt::println(stderr, "Relative addressing only within same segment! - check: {}", placeHolder.ident);
        return false;
    }
    fmt::println("  from:{}, to:{}", placeHolder.ofsRelative, identifierAddr.address);

    fmt::println("  REL: {}@{:#x} = {}@{:#x}",
        placeHolder.ident, placeHolder.segment->LoadAddress() + placeHolder.address,
        identifierAddr.segment->Name(),identifierAddr.segment->LoadAddress() + identifierAddr.address);

    int offset = 0;
    if (identifierAddr.address > placeHolder.ofsRelative) {
        // FIXME: This is a bit odd - but if we jump forward I need to add 1 to the offset..
        offset = identifierAddr.address - placeHolder.ofsRelative + 1;
    } else {
        offset = identifierAddr.address - placeHolder.ofsRelative;
    }

    placeHolder.segment->ReplaceAt(placeHolder.address, offset, placeHolder.opSize);

    return true;
}

bool ElfLinker::RelocateAbsolute(CompiledUnit &unit, IdentifierAddress &identifierAddr, IdentifierAddressPlaceholder &placeHolder) {
    fmt::println("  {}@{:#x} = {}@{:#x}",
        placeHolder.ident, placeHolder.segment->LoadAddress() + placeHolder.address,
        identifierAddr.segment->Name(),identifierAddr.segment->LoadAddress() + identifierAddr.address);

    // WEFU@#$T)&#$YTG(# - DO NOT ASSUME we only want to 'lea' from .data!!!!
    if (identifierAddr.segment == nullptr) {
        fmt::println(stderr, "Linker: no segment for identifier '{}'", placeHolder.ident);
        return false;
    }
    if (placeHolder.segment == nullptr) {
        fmt::println(stderr, "Linker, placehold has no segment - can't replace!");
        return false;
    }

    placeHolder.segment->ReplaceAt(placeHolder.address, identifierAddr.segment->LoadAddress() + identifierAddr.address);

    return true;
}

bool ElfLinker::WriteElf(CompiledUnit &unit) {
    // You can't proceed without this function call!
    elfWriter.create(ELFCLASS64, ELFDATA2LSB );

    elfWriter.set_os_abi(ELFOSABI_NONE );
    elfWriter.set_type(ET_EXEC );
    elfWriter.set_machine(EM_68K );

    auto textSeg = unit.GetSegment(".text");
    if (textSeg == nullptr) {
        return false;
    }

    section *elfTextSec = elfWriter.sections.add(".text");
    elfTextSec->set_type( SHT_PROGBITS );
    elfTextSec->set_flags( SHF_ALLOC | SHF_EXECINSTR );
    elfTextSec->set_addr_align( 0x10 );
    elfTextSec->set_data((const char *)textSeg->DataPtr(), textSeg->Size());

    segment *elfTextSeg = elfWriter.segments.add();
    elfTextSeg->set_type(PT_LOAD);
    elfTextSeg->set_virtual_address(textSeg->LoadAddress());
    elfTextSeg->set_physical_address(textSeg->LoadAddress());
    elfTextSeg->set_flags(PF_X | PF_R);
    elfTextSeg->set_align(vcpu::VCPU_MMU_PAGE_SIZE);
    elfTextSeg->add_section(elfTextSec, elfTextSec->get_addr_align());

    auto dataSeg = unit.GetSegment(".data");
    // FIXME: this is not an error...
    if (dataSeg == nullptr) {
        fmt::println(stderr,"ElfLinker, no data segment!");
        return false;
    }
    section *elfDataSec = elfWriter.sections.add(".data");
    elfDataSec->set_type(SHT_PROGBITS);
    elfDataSec->set_flags(SHF_ALLOC | SHF_WRITE);
    elfDataSec->set_addr_align(0x04);   // verify!
    elfDataSec->set_data((const char *)dataSeg->DataPtr(), dataSeg->Size());

    segment *elfDataSeg = elfWriter.segments.add();
    elfDataSeg->set_type(PT_LOAD);
    elfDataSeg->set_virtual_address(dataSeg->LoadAddress());
    elfDataSeg->set_physical_address(dataSeg->LoadAddress());
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
