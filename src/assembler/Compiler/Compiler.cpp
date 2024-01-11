//
// Created by gnilk on 15.12.23.
//

#include "Compiler.h"
#include "InstructionSet.h"

#include "fmt/format.h"

#include <memory>
#include <unordered_map>

#include "InstructionSet.h"
#include "InstructionSet.h"
#include "Linker/CompiledUnit.h"

#include "elfio/elfio.hpp"
#include "Linker/DummyLinker.h"
#include "Linker/ElfLinker.h"

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
    //DummyLinker linker;
    ElfLinker linker;
    return linker.Link(unit, identifierAddresses, addressPlaceholders);
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
        case ast::NodeType::kStructStatement :
            return ProcessStructStatement(std::dynamic_pointer_cast<ast::StructStatement>(stmt));
        case ast::NodeType::kStructLiteral :
            return ProcessStructLiteral(std::dynamic_pointer_cast<ast::StructLiteral>(stmt));
        case ast::NodeType::kConstLiteral :
            return ProcessConstLiteral(std::dynamic_pointer_cast<ast::ConstLiteral>(stmt));
        default :
            fmt::println(stderr, "Compiler, unknown statment");
    }
    return false;
}

bool Compiler::ProcessStructStatement(ast::StructStatement::Ref stmt) {

    auto &members = stmt->Declarations();
    size_t nBytes = 0;
    for(auto &m : members) {
        if (m->Kind() != ast::NodeType::kReservationStatment) {
            fmt::println(stderr, "Compiler, struct definition should only contain reservation statments");
            return false;
        }
        auto reservation = std::dynamic_pointer_cast<ast::ReservationStatement>(m);
        auto elemLiteral = EvaluateConstantExpression(reservation->NumElements());
        if (elemLiteral->Kind() != ast::NodeType::kNumericLiteral) {
            fmt::println(stderr, "Compiler, reservation expression did not yield a numerical result!");
            return false;
        }
        auto numElem = std::dynamic_pointer_cast<ast::NumericLiteral>(elemLiteral);
        auto szPerElem = vcpu::ByteSizeOfOperandSize(reservation->OperandSize());

        nBytes += numElem->Value() * szPerElem;
    }

    StructDefinition structDefinition  = {
        .ident = stmt->Name(),
        .byteSize = nBytes,
    };
    structDefinitions.push_back(structDefinition);

    return true;
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

        auto seg = unit.GetActiveSegment();
        if (seg->LoadAddress() == 0) {
            seg->SetLoadAddress(numArg->Value());
        } else {
            // A segment can only have one base-address, in case we issue '.org' twice within the same segment - we duplicate the segment...
            fmt::println(stderr, "Compiler, segment already have base address - can have .org statement twice within same segment");
        }

        return true;
    }

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
        switch(exp->Kind()) {
            case ast::NodeType::kNumericLiteral :
                if (!EmitNumericLiteral(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(exp))) {
                    return false;
                }
                break;
            case ast::NodeType::kStringLiteral :
                if (!EmitStringLiteral(opSize, std::dynamic_pointer_cast<ast::StringLiteral>(exp))) {
                    return false;
                }
                break;
            default:
                fmt::println("Compiler, Only numeric and string literals are supported for arrays");
                return false;
        }

    }
    return true;
}

bool Compiler::ProcessConstLiteral(ast::ConstLiteral::Ref stmt) {
    if (constants.contains(stmt->Ident())) {
        fmt::println(stderr, "Compiler, const with name '{}' already defined", stmt->Ident());
        return false;
    }
    constants[stmt->Ident()] = stmt;
    return true;
}


bool Compiler::ProcessStructLiteral(ast::StructLiteral::Ref stmt) {

    bool bFoundStructType = false;
    size_t szExpected = 0;
    for(auto &structDef : structDefinitions) {
        if (structDef.ident == stmt->StructTypeName()) {
            szExpected = structDef.byteSize;
            bFoundStructType = true;
            break;
        }
    }

    if (!bFoundStructType) {
        fmt::println(stderr, "Compiler, undefined struct type '{}'", stmt->StructTypeName());
        return false;
    }

    auto writeStart = unit.GetCurrentWritePtr();
    for (auto &m : stmt->Members()) {
        ProcessStmt(m);
    }
    auto nBytesGenerated =  unit.GetCurrentWritePtr() - writeStart;

    // This is no error as such or is it??
    if (nBytesGenerated > szExpected) {
        fmt::println(stderr, "Compiler, too many elements in struct declaration");
        return true;
    }

    if (nBytesGenerated < szExpected) {
        fmt::println(stderr, "Compiler, not enough bytes, filling with zero");
        while(nBytesGenerated < szExpected) {
            EmitByte(0x00);
            nBytesGenerated++;
        }
        return true;
    }

    // Perfect - structure definintion matches instance...

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
    uint8_t opSizeAndFamilyCode = static_cast<uint8_t>(opSize);
    opSizeAndFamilyCode |= static_cast<uint8_t>(twoOpInstr->OpFamily()) << 4;
    if (!EmitByte(opSizeAndFamilyCode)) {
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

//
// Evaluate a constant expression...
//
ast::Literal::Ref Compiler::EvaluateConstantExpression(ast::Expression::Ref expression) {
    switch (expression->Kind()) {
        case ast::NodeType::kNumericLiteral :
            return std::dynamic_pointer_cast<ast::Literal>(expression);
        case ast::NodeType::kBinaryExpression :
            return EvaluateBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
        case ast::NodeType::kDeRefExpression : {
                // If the expression starts with '(' this will be marked as a de-reference expression
                // let's just forward the internal expression in that case and hope for the best...
                auto deRefExp = std::dynamic_pointer_cast<ast::DeReferenceExpression>(expression);
                return EvaluateConstantExpression(deRefExp->GetDeRefExp());
            }
        case ast::NodeType::kRegisterLiteral :
            return std::dynamic_pointer_cast<ast::Literal>(expression);
        case ast::NodeType::kIdentifier : {
            auto identifier = std::dynamic_pointer_cast<ast::Identifier>(expression);
            if (constants.contains(identifier->Symbol())) {
                auto constLiteral = constants[identifier->Symbol()];
                return EvaluateConstantExpression(constLiteral->Expression());
            }
            return nullptr;
        }
        case ast::NodeType::kStringLiteral :
            fmt::println(stderr, "Compiler, string literals as constants are not yet supported!");
            exit(1);
        default :
            fmt::println(stderr, "Compiler, Unsupported node type: {} in constant expression", ast::NodeTypeToString(expression->Kind()));
            exit(1);
    }
    return {};
}

ast::Literal::Ref Compiler::EvaluateBinaryExpression(ast::BinaryExpression::Ref expression) {
    auto lhs = EvaluateConstantExpression(expression->Left());
    auto rhs = EvaluateConstantExpression(expression->Right());
    if ((lhs->Kind() == ast::NodeType::kRegisterLiteral) && (rhs->Kind() == ast::NodeType::kRegisterLiteral)) {

        auto literal = ast::RelativeRegisterLiteral::Create(std::dynamic_pointer_cast<ast::RegisterLiteral>(lhs),
            std::dynamic_pointer_cast<ast::RegisterLiteral>(rhs));

        return literal;
    }

    if ((lhs->Kind() == ast::NodeType::kRegisterLiteral) && (rhs->Kind() == ast::NodeType::kNumericLiteral)) {

        auto literal = ast::RelativeRegisterLiteral::Create(std::dynamic_pointer_cast<ast::RegisterLiteral>(lhs),
            std::dynamic_pointer_cast<ast::NumericLiteral>(rhs));

        literal->SetOperator(expression->Operator());

        return literal;
    }

    if ((lhs->Kind() == ast::NodeType::kRegisterLiteral) && (rhs->Kind() == ast::NodeType::kRelativeRegisterLiteral)) {

        auto literal = ast::RelativeRegisterLiteral::Create(std::dynamic_pointer_cast<ast::RegisterLiteral>(lhs),
            std::dynamic_pointer_cast<ast::Literal>(rhs));


        literal->SetOperator(expression->Operator());

        return literal;
    }

    // Is this a constant expression???
    if ((lhs->Kind() == ast::NodeType::kNumericLiteral) && (rhs->Kind() == ast::NodeType::kNumericLiteral)) {
        auto leftLValue = std::dynamic_pointer_cast<ast::NumericLiteral>(lhs);
        auto rightLValue = std::dynamic_pointer_cast<ast::NumericLiteral>(rhs);
        auto oper = expression->Operator();
        if (oper == "+") {
            return std::make_shared<ast::NumericLiteral>(leftLValue->Value() + rightLValue->Value());
        }
        if (oper == "-") {
            return std::make_shared<ast::NumericLiteral>(leftLValue->Value() - rightLValue->Value());
        }
        if (oper == "*") {
            return std::make_shared<ast::NumericLiteral>(leftLValue->Value() * rightLValue->Value());
        }
        if (oper == "/") {
            return std::make_shared<ast::NumericLiteral>(leftLValue->Value() / rightLValue->Value());
        }

    }


    fmt::println(stderr, "Compiler, EvaluateBinaryExpression, lhs/rhs combination not supported");

    return nullptr;
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
    {"cr0",8},
    {"cr1",9},
    {"cr2",10},
    {"cr3",11},
    {"cr4",12},
    {"cr5",13},
    {"cr6",14},
    {"cr7",15},
};


bool Compiler::EmitInstrOperand(vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref operandExp) {

    // If this is an identifier - check if it is actually a constant and work out the actual value - recursively
    if (operandExp->Kind() == ast::NodeType::kIdentifier) {
        auto identifier = std::dynamic_pointer_cast<ast::Identifier>(operandExp);
        if (constants.contains(identifier->Symbol())) {
            auto constLiteral = constants[identifier->Symbol()];
            auto constExpression = EvaluateConstantExpression(constLiteral->Expression());
            return EmitInstrOperand(desc, opSize, constExpression);
        }
    }

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
            if ((desc.features & vcpu::OperandDescriptionFlags::Addressing) && (opSize == vcpu::OperandSize::Long)) {
                return EmitLabelAddress(std::dynamic_pointer_cast<ast::Identifier>(operandExp));
            } else {
                return EmitRelativeLabelAddress(std::dynamic_pointer_cast<ast::Identifier>(operandExp), opSize);
            }
            break;
        case ast::NodeType::kDeRefExpression :
            if (desc.features & vcpu::OperandDescriptionFlags::Addressing) {
                return EmitDereference(std::dynamic_pointer_cast<ast::DeReferenceExpression>(operandExp));
            }
            break;
        default :
            fmt::println(stderr, "Unsupported instr. operand type in AST");
            break;
    }
    return false;
}

// This is a bit hairy - to say the least
bool Compiler::EmitDereference(ast::DeReferenceExpression::Ref expression) {

    auto deref = EvaluateConstantExpression(expression->GetDeRefExp());

    // Regular dereference, (<reg>)
    if (deref->Kind() == ast::NodeType::kRegisterLiteral) {
        return EmitRegisterLiteralWithAddrMode(std::dynamic_pointer_cast<ast::RegisterLiteral>(deref), vcpu::AddressMode::Indirect);
    }

    // Check if we have a relative register dereference;  (reg + <expression>)
    if (deref->Kind() == ast::NodeType::kRelativeRegisterLiteral) {
        const auto relativeRegLiteral = std::dynamic_pointer_cast<ast::RelativeRegisterLiteral>(deref);

        // Relative to a register?
        if (relativeRegLiteral->RelativeExpression()->Kind() == ast::NodeType::kRegisterLiteral) {
            // Setup address mode
            int addrMode = vcpu::AddressMode::Indirect;
            addrMode |= (vcpu::RelativeAddressMode::RegRelative) << 2;
            // Output base register
            if (!EmitRegisterLiteralWithAddrMode(relativeRegLiteral->BaseRegister(), addrMode)) {
                return false;
            }
            // Output the relative register (mode is discared in this case)
            return EmitRegisterLiteralWithAddrMode(std::dynamic_pointer_cast<ast::RegisterLiteral>(relativeRegLiteral->RelativeExpression()), 0);
        }
        // Relative to an absolute value; (reg + <number>)
        if (relativeRegLiteral->RelativeExpression()->Kind() == ast::NodeType::kNumericLiteral) {
            int addrMode = vcpu::AddressMode::Indirect;
            addrMode |= (vcpu::RelativeAddressMode::AbsRelative) << 2;
            if (!EmitRegisterLiteralWithAddrMode(relativeRegLiteral->BaseRegister(), addrMode)) {
                return false;
            }

            const auto relLiteral = std::dynamic_pointer_cast<ast::NumericLiteral>(relativeRegLiteral->RelativeExpression());
            if (relLiteral->Value() > 255) {
                fmt::println(stderr, "Compiler, Absolute relative value {} out range - must be within 0..255", relLiteral->Value());
                return false;
            }
            return EmitByte(relLiteral->Value());
        }

        // Handle shift values...   (a0+d1<<1)
        if (relativeRegLiteral->RelativeExpression()->Kind() == ast::NodeType::kRelativeRegisterLiteral) {

            // 1 - output the base register tag as reg-relative
            // this is the '(a0..+'  part of the expressions
            int addrMode = vcpu::AddressMode::Indirect;
            addrMode |= (vcpu::RelativeAddressMode::RegRelative) << 2;
            // Output base register
            if (!EmitRegisterLiteralWithAddrMode(relativeRegLiteral->BaseRegister(), addrMode)) {
                return false;
            }

            // 2 - calculate and output the relative part with scaling
            // this is the '(.. + d3<<1)'  part of the expressions
            const auto relRegShift = std::dynamic_pointer_cast<ast::RelativeRegisterLiteral>(relativeRegLiteral->RelativeExpression());

            // Check the operator - the parser is pretty dumb in this case - as I reuse the normal 'binary' expressions
            if (relRegShift->Operator() != "<<") {
                fmt::println(stderr, "Compiler, only '<<' operator is supported for relative register scaling!");
                return false;
            }

            // Also check that we have a number; we don't want someone doing '(... + d1 << d2)'
            if (relRegShift->RelativeExpression()->Kind() != ast::NodeType::kNumericLiteral) {
                fmt::println(stderr, "Compiler, relative register scaling must be a numerical value!");
                return false;
            }
            auto relRegShiftValue = std::dynamic_pointer_cast<ast::NumericLiteral>(relRegShift->RelativeExpression());
            auto shiftNum = relRegShiftValue->Value();

            // we only have 4 bits for the shift number - but I guess that range is sufficient for now...
            // because shift number is the lower 4 bits of the reg-index; 'RRRR | SSSS' in this mode..
            if (shiftNum > 15) {
                fmt::println(stderr, "Compiler, relative register scaling too high {}, allowed range is 0..15",shiftNum);
                return false;
            }

            addrMode = shiftNum;
            if (!EmitRegisterLiteralWithAddrMode(relRegShift->BaseRegister(), addrMode)) {
                return false;
            }

            return true;

        }

    }

    fmt::println(stderr, "Compiler, Unsupported dereference construct!");
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
            return EmitNumericLiteralForInstr(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(src));
        case ast::NodeType::kRegisterLiteral :
            return EmitRegisterLiteral(std::dynamic_pointer_cast<ast::RegisterLiteral>(src));
        default :
            fmt::println(stderr, "Unsupported instr. src type in AST");
            break;
    }
    return false;
}

bool Compiler::EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral) {
    return EmitRegisterLiteralWithAddrMode(regLiteral, vcpu::AddressMode::Register);
}

bool Compiler::EmitRegisterLiteralWithAddrMode(ast::RegisterLiteral::Ref regLiteral, uint8_t addrMode) {
    if (!regToIdx.contains(regLiteral->Symbol())) {
        fmt::println(stderr, "Illegal register: {}", regLiteral->Symbol());
        return false;
    }

    auto idxReg = regToIdx[regLiteral->Symbol()];
    // Register|Mode = byte = RRRR | MMMM
    auto regMode = (idxReg << 4) & 0xf0;
    regMode |= addrMode;

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

bool Compiler::EmitStringLiteral(vcpu::OperandSize opSize, ast::StringLiteral::Ref strLiteral) {
    for(auto &v : strLiteral->Value()) {
        EmitByte(v);
    }
    return true;
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

    // This is an absolute jump
    regMode |= vcpu::AddressMode::Absolute;

    // Register|Mode = byte = RRRR | MMMM
    EmitByte(regMode);

    IdentifierAddressPlaceholder addressPlaceholder;
    addressPlaceholder.segment = unit.GetActiveSegment();
    addressPlaceholder.address = static_cast<uint64_t>(unit.GetCurrentWritePtr());
    addressPlaceholder.ident = identifier->Symbol();
    addressPlaceholder.isRelative = false;
    addressPlaceholders.push_back(addressPlaceholder);

    EmitLWord(0);   // placeholder
    return true;
}

bool Compiler::EmitRelativeLabelAddress(ast::Identifier::Ref identifier, vcpu::OperandSize opSize) {
    uint8_t regMode = 0; // no register

    // This a relative jump
    regMode |= vcpu::AddressMode::Immediate;

    // Register|Mode = byte = RRRR | MMMM
    EmitByte(regMode);

    IdentifierAddressPlaceholder addressPlaceholder;
    addressPlaceholder.segment = unit.GetActiveSegment();

    addressPlaceholder.address = static_cast<uint64_t>(unit.GetCurrentWritePtr());
    addressPlaceholder.ident = identifier->Symbol();
    addressPlaceholder.isRelative = true;
    addressPlaceholder.opSize = opSize;
    // We are always relative to the complete instruction
    // Note: could have been relative to the start of the instr. but that would require a state (or caching of the IP) in the cpu
    addressPlaceholder.ofsRelative = static_cast<uint64_t>(unit.GetCurrentWritePtr()) + vcpu::ByteSizeOfOperandSize(opSize);

    addressPlaceholders.push_back(addressPlaceholder);

    switch(opSize) {
        case vcpu::OperandSize::Byte :
            EmitByte(0);
            break;
        case vcpu::OperandSize::Word :
            EmitWord(0);
            break;
        case vcpu::OperandSize::DWord :
            EmitDWord(0);
            break;
        default :
            return false;
    }

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


