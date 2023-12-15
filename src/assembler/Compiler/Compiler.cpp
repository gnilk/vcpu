//
// Created by gnilk on 15.12.23.
//

#include "Compiler.h"
#include "VirtualCPU.h"

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
    return true;
}

bool Compiler::ProcessStmt(ast::Statement::Ref stmt) {
    switch(stmt->Kind()) {
        case ast::NodeType::kMoveInstrStatement :
            return ProcessMoveInstr(std::dynamic_pointer_cast<ast::MoveInstrStatment>(stmt));
    }
    return false;
}

bool Compiler::ProcessMoveInstr(ast::MoveInstrStatment::Ref moveInstr) {
    if (!EmitOpCodeForSymbol(moveInstr->Symbol())) {
        return false;
    }
    auto opSize = moveInstr->OpSize();
    if (!EmitByte(opSize)) {
        return false;
    }
    if (!EmitInstrDst(opSize, moveInstr->Dst())) {
        return false;
    }
    if (!EmitInstrSrc(opSize, moveInstr->Src())) {
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

bool Compiler::EmitInstrDst(OperandSize opSize, ast::Expression::Ref dst) {
    if (dst->Kind() != ast::NodeType::kRegisterLiteral) {
        fmt::println("Only registers supported as destination!");
        return false;
    }
    return EmitRegisterLiteral(std::dynamic_pointer_cast<ast::RegisterLiteral>(dst));
}

bool Compiler::EmitInstrSrc(OperandSize opSize, ast::Expression::Ref src) {
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
    regMode |= AddressMode::Register;

    EmitByte(regMode);
    return true;
}

bool Compiler::EmitNumericLiteral(OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {
    uint8_t regMode = 0; // no register
    regMode |= AddressMode::Immediate;

    // Register|Mode = byte = RRRR | MMMM
    EmitByte(regMode);

    switch(opSize) {
        case OperandSize::Byte :
            return EmitByte(numLiteral->Value());
        case OperandSize::Word :
            return EmitWord(numLiteral->Value());
        case OperandSize::DWord :
            return EmitDWord(numLiteral->Value());
        case OperandSize::Long :
            return EmitLWord(numLiteral->Value());
        default :
            fmt::println(stderr, "Only byte size supported!");
    }
    return false;
}



static std::unordered_map<std::string, OperandClass> symbolToOpCode = {
    {"move", OperandClass::MOV},
    {"add", OperandClass::ADD},
};

bool Compiler::EmitOpCodeForSymbol(const std::string &symbol) {
    if (!symbolToOpCode.contains(symbol)) {
        fmt::println(stderr, "Unknown/Unsupported symbol: {}", symbol);
        return false;
    }
    return EmitByte(symbolToOpCode[symbol]);
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
