//
// Created by gnilk on 15.12.23.
//

#include "Compiler.h"
#include "InstructionSet.h"

#include <memory>
#include <unordered_map>

using namespace gnilk;
using namespace gnilk::assembler;



bool Compiler::GenerateCode(ast::Program::Ref program) {
    outStream.clear();
    for(auto &statement : program->Body()) {
        if (!ProcessStmt(statement)) {
            return false;
        }
    }
    // Do we have placeholds??
    if ((!addressPlaceholders.empty()) && (outStream.size() < sizeof(uint64_t))) {
        fmt::println(stderr,"Invalid compiled size, can't possible hold address to anything");
        return false;
    }

    return ReplaceIdentPlaceholdersWithAddresses();
}

//
// This replaces identifier placesholders for instructions with their real address
// When an instruction has a reference to an identifier the compiler inserts an absolute placeholder address and
// records the identifier and location of the placeholder, in the 'addressPlaceholders' array...
//
// Same sense, when the compiler encounters an identifier, the address is stored for that identifier ('identifierAddresses')
//
// After the file has been compiled - all place-holder addresses are replaced with the proper address of the identifier
//
bool Compiler::ReplaceIdentPlaceholdersWithAddresses() {
    for(auto &placeHolder : addressPlaceholders) {
        if (!identifierAddresses.contains(placeHolder.ident)) {
            fmt::println(stderr, "Unknown identifier: {}", placeHolder.ident);
            return false;
        }
        const uint64_t addr = identifierAddresses[placeHolder.ident];
        uint64_t ofs = placeHolder.address;
        if (ofs > (outStream.size() - 8)) {
            fmt::println("Binary size invalid - offset would overflow end binary code!");
            return false;
        }
        outStream[ofs++] = (addr>>56 & 0xff);
        outStream[ofs++] = (addr>>48 & 0xff);
        outStream[ofs++] = (addr>>40 & 0xff);
        outStream[ofs++] = (addr>>32 & 0xff);
        outStream[ofs++] = (addr>>24 & 0xff);
        outStream[ofs++] = (addr>>16 & 0xff);
        outStream[ofs++] = (addr>>8 & 0xff);
        outStream[ofs++] = (addr & 0xff);
    }
    return true;
}

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
    }
    return false;
}

bool Compiler::ProcessIdentifier(ast::Identifier::Ref identifier) {
    uint64_t ipNow = static_cast<uint64_t>(outStream.size());
    if (identifierAddresses.contains(identifier->Symbol())) {
        fmt::println(stderr,"Identifier {} already in use - can't use duplicate identifiers!", identifier->Symbol());
        return false;
    }
    identifierAddresses[identifier->Symbol()] = ipNow;
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

    if (!EmitByte(opSize)) {
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
    if (!EmitByte(opSize)) {
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
                return EmitNumericLiteral(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(operandExp));
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
            // FIXME: emit byte+placeholder (uint64_t), record IP in struct (incl. which identifier we depend upon)
            // After statement parsing is complete we will change all place-holders
            if (desc.features & vcpu::OperandDescriptionFlags::Addressing) {
                return EmitLabelAddress(std::dynamic_pointer_cast<ast::Identifier>(operandExp));
            }
            break;
    }
    return false;
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
            return EmitNumericLiteral(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(src));
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
    auto idxReg = regToIdx[regLiteral->Symbol()];
    // Register|Mode = byte = RRRR | MMMM
    auto regMode = (idxReg << 4) & 0xf0;
    regMode |= vcpu::AddressMode::Register;

    EmitByte(regMode);
    return true;
}

bool Compiler::EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {
    uint8_t regMode = 0; // no register
    regMode |= vcpu::AddressMode::Immediate;

    // Register|Mode = byte = RRRR | MMMM
    EmitByte(regMode);

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
    addressPlaceholder.address = static_cast<uint64_t>(outStream.size());
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
    outStream.push_back(byte);
    return true;
}

bool Compiler::EmitWord(uint16_t word) {
    outStream.push_back((word >> 8) & 0xff);
    outStream.push_back(word & 0xff);
    return true;
}
bool Compiler::EmitDWord(uint32_t dword) {
    outStream.push_back((dword >> 24) & 0xff);
    outStream.push_back((dword >> 16) & 0xff);
    outStream.push_back((dword >> 8) & 0xff);
    outStream.push_back(dword & 0xff);
    return true;
}
bool Compiler::EmitLWord(uint64_t lword) {
    outStream.push_back((lword >> 56) & 0xff);
    outStream.push_back((lword >> 48) & 0xff);
    outStream.push_back((lword >> 40) & 0xff);
    outStream.push_back((lword >> 32) & 0xff);
    outStream.push_back((lword >> 24) & 0xff);
    outStream.push_back((lword >> 16) & 0xff);
    outStream.push_back((lword >> 8) & 0xff);
    outStream.push_back(lword & 0xff);
    return true;
}
