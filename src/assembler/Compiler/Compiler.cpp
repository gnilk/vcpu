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
    // FIXME: Go through all statements which are dependent on labels/identifiers and update them
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
            // FIXME: Need to process this here..
            // Label should just record the current IP position and create a label-record
            return true;
    }
    return false;
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
