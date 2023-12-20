//
// Created by gnilk on 15.12.23.
//

#include "Compiler.h"
#include "InstructionSet.h"

#include "fmt/format.h"

#include <memory>
#include <unordered_map>

using namespace gnilk;
using namespace gnilk::assembler;



bool Compiler::CompileAndLink(ast::Program::Ref program) {
    if (!Compile(program)) {
        return false;
    }
    return Link();
}
bool Compiler::Compile(gnilk::ast::Program::Ref program) {
    unit.Clear();
    // TEMP!
    unit.GetOrAddSegment(".text", 0);

    for(auto &statement : program->Body()) {
        if (!ProcessStmt(statement)) {
            return false;
        }
    }
    return true;
}

//
// This is a simple single-unit self-linking thingie - produces something like a a.out file...
// We should split this to it's own structure
//
bool Compiler::Link() {

    // Merge segments (.text and .data) to hidden 'link_out' segment...
    unit.MergeSegments("link_out", ".text");
    size_t ofsDataSegStart = unit.GetSegmentSize("link_out");

    // This will just fail if '.data' doesnt' exists..
    if (!unit.MergeSegments("link_out", ".data")) {
        fmt::println("No data segment, merging directly into '.text'");
        // No data segment, well - we reset this one in that case...
        ofsDataSegStart = 0;
    }

    // This is our target now...
    unit.SetActiveSegment("link_out");

    auto baseAddress = unit.GetBaseAddress();

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

        fmt::println("  {}@{:#x} = {}@{:#x}",placeHolder.ident, placeHolder.address - baseAddress, identifierAddr.segment,identifierAddr.address + ofsDataSegStart);

        unit.ReplaceAt(placeHolder.address - baseAddress, identifierAddr.address + ofsDataSegStart);
    }
    return true;
}

//
// AST processing starts here...
//
bool Compiler::ProcessStmt(ast::Statement::Ref stmt) {
    switch(stmt->Kind()) {
        case ast::NodeType::kNoOpInstrStatement :
            return ProcessNoOpInstrStmt(std::dynamic_pointer_cast<ast::NoOpInstrStatment>(stmt));
        case ast::NodeType::kOneOpInstrStatement :
            return ProcessOneOpInstrStmt(std::dynamic_pointer_cast<ast::OneOpInstrStatment>(stmt));
        case ast::NodeType::kTwoOpInstrStatement :
            return ProcessTwoOpInstrStmt(std::dynamic_pointer_cast<ast::TwoOpInstrStatment>(stmt));
        case ast::NodeType::kIdentifier :
            return ProcessIdentifier(std::dynamic_pointer_cast<ast::Identifier>(stmt));
        case ast::NodeType::kArrayLiteral :
            return ProcessArrayLiteral(std::dynamic_pointer_cast<ast::ArrayLiteral>(stmt));
        case ast::NodeType::kMetaStatement :
            return ProcessMetaStatement(std::dynamic_pointer_cast<ast::MetaStatement>(stmt));
        case ast::NodeType::kCommentStatement :
            return true;
    }
    return false;
}
bool Compiler::ProcessMetaStatement(ast::MetaStatement::Ref stmt) {
    if (stmt->Symbol() == "org") {
        auto arg = stmt->Argument();
        if (arg == nullptr) {
            fmt::println(stderr, ".org directive requires expression");
            return false;
        }
        if (arg->Kind() != ast::NodeType::kNumericLiteral) {
            fmt::println(stderr, ".org expects numeric expression");
            return false;
        }
        auto numArg = std::dynamic_pointer_cast<ast::NumericLiteral>(arg);
        unit.SetBaseAddress(numArg->Value());
        return true;
    }

    // FIXME: Addresses...

    if ((stmt->Symbol() == "text") || (stmt->Symbol() == "code")) {
        unit.GetOrAddSegment(".text",0);
        return true;
    }
    if (stmt->Symbol() == "data") {
        unit.GetOrAddSegment(".data",0);
        return true;
    }
    if (stmt->Symbol() == "bss") {
        unit.GetOrAddSegment(".bss",0);
        return true;
    }

    return true;
}
bool Compiler::ProcessArrayLiteral(ast::ArrayLiteral::Ref stmt) {
    auto opSize = stmt->OpSize();
    auto &array = stmt->Array();

    for(auto &exp : array) {
        if (exp->Kind() != ast::NodeType::kNumericLiteral) {
            fmt::println("Only numeric literals support for arrays");
            return false;
        }
        if (!EmitNumericLiteral(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(exp))) {
            return false;
        }
    }
    return true;
}

bool Compiler::ProcessIdentifier(ast::Identifier::Ref identifier) {

    uint64_t ipNow = unit.GetCurrentWritePtr();

    if (identifierAddresses.contains(identifier->Symbol())) {
        fmt::println(stderr,"Identifier {} already in use - can't use duplicate identifiers!", identifier->Symbol());
        return false;
    }
    IdentifierAddress idAddress = {.segment = unit.GetActiveSegment(), .address = ipNow};
    identifierAddresses[identifier->Symbol()] = idAddress;
    return true;
}

bool Compiler::ProcessNoOpInstrStmt(ast::NoOpInstrStatment::Ref stmt) {
    return EmitOpCodeForSymbol(stmt->Symbol());
}

bool Compiler::ProcessOneOpInstrStmt(ast::OneOpInstrStatment::Ref stmt) {
    if (!EmitOpCodeForSymbol(stmt->Symbol())) {
        return false;
    }
    auto opSize = stmt->OpSize();

    if (!EmitByte(static_cast<uint8_t>(opSize))) {
        return false;
    }
    // FIXME: Cache this in the parser stage(?)
    auto opClass = *vcpu::GetOperandFromStr(stmt->Symbol());
    auto opDesc = *vcpu::GetOpDescFromClass(opClass);

    if (!EmitInstrOperand(opDesc, opSize, stmt->Operand())) {
        return false;
    }

    return true;

}

bool Compiler::ProcessTwoOpInstrStmt(ast::TwoOpInstrStatment::Ref twoOpInstr) {
    if (!EmitOpCodeForSymbol(twoOpInstr->Symbol())) {
        return false;
    }
    auto opSize = twoOpInstr->OpSize();
    if (!EmitByte(static_cast<uint8_t>(opSize))) {
        return false;
    }
    // FIXME: Cache this in the parser stage(?)
    auto opClass = *vcpu::GetOperandFromStr(twoOpInstr->Symbol());
    auto opDesc = *vcpu::GetOpDescFromClass(opClass);

    if (!EmitInstrOperand(opDesc, opSize, twoOpInstr->Dst())) {
        return false;
    }
    if (!EmitInstrOperand(opDesc, opSize, twoOpInstr->Src())) {
        return false;
    }

    return true;
}

// Note: these are 'raw' values - in order to have them in the REGMODE byte of an instr. they must be shifted
//       see 'EmitRegisterLiteral'
static std::unordered_map<std::string, uint8_t> regToIdx = {
    {"d0",0},
    {"d1",1},
    {"d2",2},
    {"d3",3},
    {"d4",4},
    {"d5",5},
    {"d6",6},
    {"d7",7},
    {"a0",8},
    {"a1",9},
    {"a2",10},
    {"a3",11},
    {"a4",12},
    {"a5",13},
    {"a6",14},
    {"a7",15},
};


bool Compiler::EmitInstrOperand(vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref operandExp) {
    switch(operandExp->Kind()) {
        case ast::NodeType::kNumericLiteral :
            if (desc.features & vcpu::OperandDescriptionFlags::Immediate) {
                return EmitNumericLiteralForInstr(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(operandExp));
            }
            fmt::println(stderr, "Instruction Operand does not support immediate");
            break;
        case ast::NodeType::kRegisterLiteral :
            if (desc.features & vcpu::OperandDescriptionFlags::Register) {
                return EmitRegisterLiteral(std::dynamic_pointer_cast<ast::RegisterLiteral>(operandExp));
            }
            fmt::println(stderr, "Instruction Operand does not support register");
            break;
        case ast::NodeType::kIdentifier :
            // After statement parsing is complete we will change all place-holders
            if (desc.features & vcpu::OperandDescriptionFlags::Addressing) {
                return EmitLabelAddress(std::dynamic_pointer_cast<ast::Identifier>(operandExp));
            }
            break;
        case ast::NodeType::kDeRefExpression :
            if (desc.features & vcpu::OperandDescriptionFlags::Addressing) {
                return EmitDereference(std::dynamic_pointer_cast<ast::DeReferenceExpression>(operandExp));
            }
            break;
    }
    return false;
}

bool Compiler::EmitDereference(ast::DeReferenceExpression::Ref expression) {

    auto deref = expression->GetDeRefExp();
    if (deref->Kind() != ast::NodeType::kRegisterLiteral) {
        fmt::println("Only register deref possible at the moment!");
        return false;
    }
    auto regLiteral = std::dynamic_pointer_cast<ast::RegisterLiteral>(deref);
    if (!regToIdx.contains(regLiteral->Symbol())) {
        fmt::println(stderr, "Illegal register: {}", regLiteral->Symbol());
        return false;
    }

    auto idxReg = regToIdx[regLiteral->Symbol()];
    // Register|Mode = byte = RRRR | MMMM
    auto regMode = (idxReg << 4) & 0xf0;
    regMode |= vcpu::AddressMode::Indirect;

    EmitByte(regMode);
    return true;
}


bool Compiler::EmitInstrDst(vcpu::OperandSize opSize, ast::Expression::Ref dst) {
    if (dst->Kind() != ast::NodeType::kRegisterLiteral) {
        fmt::println("Only registers supported as destination!");
        return false;
    }
    return EmitRegisterLiteral(std::dynamic_pointer_cast<ast::RegisterLiteral>(dst));
}

bool Compiler::EmitInstrSrc(vcpu::OperandSize opSize, ast::Expression::Ref src) {
    switch(src->Kind()) {
        case ast::NodeType::kNumericLiteral :
            return EmitNumericLiteralForInstr(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(src));
        case ast::NodeType::kRegisterLiteral :
            return EmitRegisterLiteral(std::dynamic_pointer_cast<ast::RegisterLiteral>(src));
    }
    return false;
}

bool Compiler::EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral) {
    if (!regToIdx.contains(regLiteral->Symbol())) {
        fmt::println(stderr, "Illegal register: {}", regLiteral->Symbol());
        return false;
    }

    //
    // FIXME: Address mode is not obvious..
    //

    auto idxReg = regToIdx[regLiteral->Symbol()];
    // Register|Mode = byte = RRRR | MMMM
    auto regMode = (idxReg << 4) & 0xf0;
    regMode |= vcpu::AddressMode::Register;

    EmitByte(regMode);
    return true;
}

bool Compiler::EmitNumericLiteralForInstr(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {
    uint8_t regMode = 0; // no register
    regMode |= vcpu::AddressMode::Immediate;

    // Register|Mode = byte = RRRR | MMMM
    EmitByte(regMode);
    return EmitNumericLiteral(opSize, numLiteral);
}

bool Compiler::EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {

    switch(opSize) {
        case vcpu::OperandSize::Byte :
            return EmitByte(numLiteral->Value());
        case vcpu::OperandSize::Word :
            return EmitWord(numLiteral->Value());
        case vcpu::OperandSize::DWord :
            return EmitDWord(numLiteral->Value());
        case vcpu::OperandSize::Long :
            return EmitLWord(numLiteral->Value());
        default :
            fmt::println(stderr, "Only byte size supported!");
    }
    return false;
}


bool Compiler::EmitLabelAddress(ast::Identifier::Ref identifier) {
    uint8_t regMode = 0; // no register

    // FIXME: Should this be absolute???  - in principle Absolute and Immediate is the same..
    regMode |= vcpu::AddressMode::Absolute;

    // Register|Mode = byte = RRRR | MMMM
    EmitByte(regMode);

    IdentifierAddressPlaceholder addressPlaceholder;
    addressPlaceholder.segment = unit.GetActiveSegment();
    addressPlaceholder.address = static_cast<uint64_t>(unit.GetCurrentWritePtr());
    addressPlaceholder.ident = identifier->Symbol();
    addressPlaceholders.push_back(addressPlaceholder);

    EmitLWord(0);   // placeholder
    return true;
}

bool Compiler::EmitOpCodeForSymbol(const std::string &symbol) {
    auto opcode = gnilk::vcpu::GetOperandFromStr(symbol);
    if (!opcode.has_value()) {
        fmt::println(stderr, "Unknown/Unsupported symbol: {}", symbol);
        return false;
    }
    return EmitByte(*opcode);
}

bool Compiler::EmitByte(uint8_t byte) {
    unit.WriteByte(byte);
    return true;
}

bool Compiler::EmitWord(uint16_t word) {
    unit.WriteByte((word >> 8) & 0xff);
    unit.WriteByte(word & 0xff);
    return true;
}
bool Compiler::EmitDWord(uint32_t dword) {
    unit.WriteByte((dword >> 24) & 0xff);
    unit.WriteByte((dword >> 16) & 0xff);
    unit.WriteByte((dword >> 8) & 0xff);
    unit.WriteByte(dword & 0xff);
    return true;
}
bool Compiler::EmitLWord(uint64_t lword) {
    unit.WriteByte((lword >> 56) & 0xff);
    unit.WriteByte((lword >> 48) & 0xff);
    unit.WriteByte((lword >> 40) & 0xff);
    unit.WriteByte((lword >> 32) & 0xff);
    unit.WriteByte((lword >> 24) & 0xff);
    unit.WriteByte((lword >> 16) & 0xff);
    unit.WriteByte((lword >> 8) & 0xff);
    unit.WriteByte(lword & 0xff);
    return true;
}


// Compiled Unit
void CompiledUnit::Clear() {
    segments.clear();
}

bool CompiledUnit::GetOrAddSegment(const std::string &name, uint64_t address) {
    if (!segments.contains(name)) {
        auto segment = std::make_shared<Segment>(name, address);
        segments[name] = segment;
    }
    activeSegment = segments.at(name);
    return true;
}

bool CompiledUnit::SetActiveSegment(const std::string &name) {
    if (!segments.contains(name)) {
        return false;
    }
    activeSegment = segments.at(name);
    return true;
}

const std::string &CompiledUnit::GetActiveSegment() {
    if (!activeSegment) {
        static std::string invalidSeg = {};
        return invalidSeg;
    }
    return activeSegment->Name();
}

size_t CompiledUnit::GetSegmentSize(const std::string &name) {
    if (!segments.contains(name)) {
        return 0;
    }
    return segments.at(name)->Size();
}


void CompiledUnit::SetBaseAddress(uint64_t address) {
    baseAddress = address;
}

uint64_t CompiledUnit::GetBaseAddress() {
    return baseAddress;
}


bool CompiledUnit::WriteByte(uint8_t byte) {
    if (!activeSegment) {
        return false;
    }
    activeSegment->WriteByte(byte);
    return true;
}
void CompiledUnit::ReplaceAt(uint64_t offset, uint64_t newValue) {
    if (!activeSegment) {
        return;
    }
    activeSegment->ReplaceAt(offset, newValue);
}


uint64_t CompiledUnit::GetCurrentWritePtr() {
    if (!activeSegment) {
        return false;
    }
    return activeSegment->GetCurrentWritePtr() + baseAddress;
}

const std::vector<uint8_t> &CompiledUnit::Data() {
    if (!GetOrAddSegment("link_out", 0x00)) {
        static std::vector<uint8_t> empty = {};
        return empty;
    }
    return activeSegment->Data();
}

bool CompiledUnit::MergeSegments(const std::string &dst, const std::string &src) {
    // The source must exists
    if (!segments.contains(src)) {
        return false;
    }
    auto srcSeg = segments.at(src);
    if (!segments.contains(dst)) {
        if (!GetOrAddSegment(dst, srcSeg->LoadAddress())) {
            return false;
        }
    }
    auto dstSeg = segments.at(dst);
    // first merge is just a plain copy...
    dstSeg->data.insert(dstSeg->data.end(), srcSeg->data.begin(), srcSeg->data.end());
    return true;
}
