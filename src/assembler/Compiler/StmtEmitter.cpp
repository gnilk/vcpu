//
// Created by gnilk on 10.03.2024.
//

#include "StmtEmitter.h"

//
// 1) Process AST and make a flat list of EmitStatements -> this can and should be done in parallell
// 2) Process the list and compute all addresses -> MUST be sequential
// 3) Process list and perform relocation
//

using namespace gnilk;
using namespace gnilk::assembler;

size_t EmitStatementBase::currentAddress = 0;


EmitStatementBase::Ref EmitStatementBase::Create(ast::Statement::Ref statement) {

    // FIXME: This needs a lock...

    EmitStatementBase::Ref ref = nullptr;
    static size_t glbEmitId = 0;

    if (statement->Kind() == ast::NodeType::kMetaStatement) {
        ref = std::make_shared<EmitMetaStatement>();
    } else if (IsDataStatement(statement->Kind())) {
        // We can add 'StructLiteral' and 'ConstLiteral' here...
        ref = std::make_shared<EmitDataStatement>();
    } else if (statement->Kind() == ast::NodeType::kIdentifier) {
        ref = std::make_shared<EmitIdentifierStatement>();
    } else if (statement->Kind() == ast::NodeType::kStructStatement) {
        // Struct statements are different
    } else if (IsCodeStatement(statement->Kind())) {
        ref = std::make_shared<EmitCodeStatement>();
    } else {

    }

    if (ref != nullptr) {
        ref->statement = statement;
        ref->emitid = glbEmitId;
        // Not sure this should happen here - or perhaps during the finalize phase
        // Because we would need to generate the complete data in order to track this
        // which I don't want to do during the 'ProcessingPhase' - during this I just want to process
        // the AST and be dependent on any real stuff..
        ref->address = currentAddress;
        glbEmitId++;
    }

    return ref;
}

// static helper for create
bool EmitStatementBase::IsDataStatement(ast::NodeType nodeType) {
    if ((nodeType == ast::NodeType::kNumericLiteral) ||
            (nodeType == ast::NodeType::kArrayLiteral) ||
            (nodeType == ast::NodeType::kConstLiteral)) {
        return true;
    }
    return false;
}

bool EmitStatementBase::IsCodeStatement(ast::NodeType nodeType) {
    if ((nodeType ==ast::NodeType::kNoOpInstrStatement) ||
            (nodeType == ast::NodeType::kOneOpInstrStatement) ||
            (nodeType == ast::NodeType::kTwoOpInstrStatement)) {
        return true;
    }
    return false;
}

// Note: WE SHOULD NOT WRITE TO UNIT HERE - only to our local buffer
bool EmitStatementBase::EmitByte(uint8_t byte) {
    WriteByte(byte);
    return true;
}

bool EmitStatementBase::EmitWord(uint16_t word) {
    WriteByte((word >> 8) & 0xff);
    WriteByte(word & 0xff);
    return true;
}
bool EmitStatementBase::EmitDWord(uint32_t dword) {
    WriteByte((dword >> 24) & 0xff);
    WriteByte((dword >> 16) & 0xff);
    WriteByte((dword >> 8) & 0xff);
    WriteByte(dword & 0xff);
    return true;
}
bool EmitStatementBase::EmitLWord(uint64_t lword) {
    WriteByte((lword >> 56) & 0xff);
    WriteByte((lword >> 48) & 0xff);
    WriteByte((lword >> 40) & 0xff);
    WriteByte((lword >> 32) & 0xff);
    WriteByte((lword >> 24) & 0xff);
    WriteByte((lword >> 16) & 0xff);
    WriteByte((lword >> 8) & 0xff);
    WriteByte(lword & 0xff);
    return true;
}

void EmitStatementBase::WriteByte(uint8_t byte) {
    data.push_back(byte);
}


/////////////
EmitMetaStatement::EmitMetaStatement() {
    emitType = kEmitType::kMeta;
}

bool EmitMetaStatement::Process(Context &context) {
    return ProcessMetaStatement(context, std::dynamic_pointer_cast<ast::MetaStatement>(statement));
}

bool EmitMetaStatement::ProcessMetaStatement(Context &context, ast::MetaStatement::Ref stmt) {

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

        origin = numArg->Value();
        isOrigin = true;

        // TODO: move this out of here - we are not dealing with segment creation just yet
        auto seg = context.Unit().GetActiveSegment();
        if ((seg->CurrentChunk()->Size() == 0) && (seg->CurrentChunk()->LoadAddress() == numArg->Value())) {
            fmt::println("Compiler, Skipping new chunk - same load-address and it's empty (would create duplicate)");
            return true;
        }
        if ((numArg->Value() > seg->CurrentChunk()->LoadAddress()) && (numArg->Value() < (seg->CurrentChunk()->LoadAddress() + seg->CurrentChunk()->Size()))) {
            fmt::println(stderr, "Compiler, .org directive overlaps with existing chunk!");
            return false;
        }

        fmt::println("Compiler, In segment {} - creating new chunk at addr {}", seg->Name(), numArg->Value());
        seg->CreateChunk(numArg->Value());

        return true;
    }

    segmentName = stmt->Symbol();

    // TODO: Move this out of here...
    if ((stmt->Symbol() == "text") || (stmt->Symbol() == "code")) {
        context.Unit().GetOrAddSegment(".text",0);
        return true;
    }
    if (stmt->Symbol() == "data") {
        context.Unit().GetOrAddSegment(".data",0);
        return true;
    }
    if (stmt->Symbol() == "bss") {
        context.Unit().GetOrAddSegment(".bss",0);
        return true;
    }
    return false;
}
bool EmitMetaStatement::Finalize(gnilk::assembler::Context &context) {
    if (!isOrigin) {
        return true;
    }

    // Make sure we can fill upp to the origin...
    auto &out = context.Data();
    if (out.capacity() < origin) {
        out.reserve(origin);
    }
}
///////////////////
//
// Data Emitters
//
EmitDataStatement::EmitDataStatement() {
    emitType = kEmitType::kData;
}

bool EmitDataStatement::Process(gnilk::assembler::Context &context) {
    if (statement->Kind() == ast::NodeType::kConstLiteral) {
        return ProcessConstLiteral(context, std::dynamic_pointer_cast<ast::ConstLiteral>(statement));
    }
    return ProcessArrayLiteral(context, std::dynamic_pointer_cast<ast::ArrayLiteral>(statement));
}

bool EmitDataStatement::Finalize(gnilk::assembler::Context &context) {
    auto &out = context.Data();
    // Insert our data
    out.insert(out.begin(), data.begin(), data.end());
    return true;
}

bool EmitDataStatement::ProcessArrayLiteral(gnilk::assembler::Context &context, ast::ArrayLiteral::Ref stmt) {
    auto opSize = stmt->OpSize();
    auto &array = stmt->Array();

    for(auto &exp : array) {
        switch(exp->Kind()) {
            case ast::NodeType::kNumericLiteral :
                if (!ProcessNumericLiteral(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(exp))) {
                    return false;
                }
                break;
            case ast::NodeType::kStringLiteral :
                if (!ProcessStringLiteral(opSize, std::dynamic_pointer_cast<ast::StringLiteral>(exp))) {
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

bool EmitDataStatement::ProcessStringLiteral(vcpu::OperandSize opSize, ast::StringLiteral::Ref strLiteral) {
    for(auto &v : strLiteral->Value()) {
        EmitByte(v);
    }
    return true;
}

bool EmitDataStatement::ProcessNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {
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

bool EmitDataStatement::ProcessConstLiteral(Context &context, ast::ConstLiteral::Ref stmt) {

    if (context.HasConstant(stmt->Ident())) {
        fmt::println(stderr, "Compiler, const with name '{}' already defined", stmt->Ident());
        return false;
    }
    context.AddConstant(stmt->Ident(), stmt);
    return true;
}

///////////
EmitIdentifierStatement::EmitIdentifierStatement() {
    emitType = kEmitType::kIdentifier;
}

bool EmitIdentifierStatement::Process(Context &context) {
    return ProcessIdentifier(context, std::dynamic_pointer_cast<ast::Identifier>(statement));
}

bool EmitIdentifierStatement::Finalize(Context &context) {

    // The address is the current write point...
    if (!context.HasIdentifierAddress(symbol)) {
        fmt::println("No such identifier '{}'", symbol);
        return false;
    }
    auto &ident = context.GetIdentifierAddress(symbol);
    ident.address = context.Data().size();
    fmt::println("EmitIdentifier, {} @ {}", symbol, ident.address);
    return true;
}


bool EmitIdentifierStatement::ProcessIdentifier(Context &context, ast::Identifier::Ref identifier) {
    uint64_t ipNow = GetCurrentAddress();


    // Need to register here so we can bail early
    if (context.HasIdentifierAddress(identifier->Symbol())) {
        fmt::println(stderr,"Identifier {} already in use - can't use duplicate identifiers!", identifier->Symbol());
        return false;
    }

    symbol = identifier->Symbol();
    address = 0;


    // Actually we just need to make sure it is registered...
    // Not quite sure all of this is needed just yet..
    // As we now have a more linear view of the meta data..
    IdentifierAddress idAddress = {};

    context.AddIdentifierAddress(identifier->Symbol(), idAddress);
    return true;
}


EmitCodeStatement::EmitCodeStatement() {
    emitType = kEmitType::kCode;
}

bool EmitCodeStatement::Process(gnilk::assembler::Context &context) {
    if(statement->Kind() == ast::NodeType::kNoOpInstrStatement) {
        return ProcessNoOpInstrStmt(std::dynamic_pointer_cast<ast::NoOpInstrStatment>(statement));
    } else if (statement->Kind() == ast::NodeType::kOneOpInstrStatement) {
        return ProcessOneOpInstrStmt(context, std::dynamic_pointer_cast<ast::OneOpInstrStatment>(statement));
    } else if (statement->Kind() == ast::NodeType::kTwoOpInstrStatement) {
        return ProcessTwoOpInstrStmt(context, std::dynamic_pointer_cast<ast::TwoOpInstrStatment>(statement));
    }
    return false;
}

bool EmitCodeStatement::Finalize(gnilk::assembler::Context &context) {
    // Here we have quite a few things to do - need to change relocation stuff
    auto &out = context.Data();
    if (haveIdentifier) {
        // Add this identifier to the list of resolve points..
        if (!context.HasIdentifierAddress(symbol)) {
            fmt::println(stderr, "Missing identifer '{}'", symbol);
            return false;
        }
        auto &identifier = context.GetIdentifierAddress(symbol);
        identifier.resolvePoints.push_back({
            .opSize = opSize,
            // FIXME: This should be current write pointer!
            .placeholderOffset = context.Data().size() + placeholderOffset,
        });
    }

    out.insert(out.end(), data.begin(), data.end());

    return true;
}

bool EmitCodeStatement::ProcessNoOpInstrStmt(ast::NoOpInstrStatment::Ref stmt) {
    return EmitOpCodeForSymbol(stmt->Symbol());
}


bool EmitCodeStatement::ProcessOneOpInstrStmt(Context &context, ast::OneOpInstrStatment::Ref stmt) {
    if (!EmitOpCodeForSymbol(stmt->Symbol())) {
        return false;
    }

    auto opSize = stmt->OpSize();

    //
    auto opSizeWritePoint = data.size(); // Save where we are

    auto opClass = *vcpu::GetOperandFromStr(stmt->Symbol());
    auto opDesc = *vcpu::GetOpDescFromClass(opClass);

    if (!EmitInstrOperand(context, opDesc, opSize, stmt->Operand())) {
        return false;
    }

    // Was opsize genereated - in case not - insert it...
    if (!(emitFlags & kEmitFlags::kEmitOpSize)) {
        data.insert(data.begin() + opSizeWritePoint, opSize);
    }

    return true;
}

bool EmitCodeStatement::ProcessTwoOpInstrStmt(Context &context, ast::TwoOpInstrStatment::Ref twoOpInstr) {
    if (twoOpInstr->Symbol() == "lea") {
        int breakme = 1;
    }

    if (!EmitOpCodeForSymbol(twoOpInstr->Symbol())) {
        return false;
    }

    auto opSizeWritePoint = data.size();

    auto opSize = twoOpInstr->OpSize();
    uint8_t opSizeAndFamilyCode = static_cast<uint8_t>(opSize);
    opSizeAndFamilyCode |= static_cast<uint8_t>(twoOpInstr->OpFamily()) << 4;

    auto opClass = *vcpu::GetOperandFromStr(twoOpInstr->Symbol());
    auto opDesc = *vcpu::GetOpDescFromClass(opClass);

    if (!EmitInstrOperand(context, opDesc, opSize, twoOpInstr->Dst())) {
        return false;
    }
    if (!EmitInstrOperand(context, opDesc, opSize, twoOpInstr->Src())) {
        return false;
    }

    // If we didn't write the operand size and family, let's do it now...
    if (!(emitFlags & kEmitFlags::kEmitOpSize)) {
        data.insert(data.begin() + opSizeWritePoint, opSize);
    }

    return true;
}

bool EmitCodeStatement::EmitOpCodeForSymbol(const std::string &symbol) {
    auto opcode = gnilk::vcpu::GetOperandFromStr(symbol);
    if (!opcode.has_value()) {
        fmt::println(stderr, "Unknown/Unsupported symbol: {}", symbol);
        return false;
    }
    emitFlags |= kEmitFlags::kEmitOpCode;
    return EmitByte(*opcode);
}

void EmitCodeStatement::EmitOpSize(uint8_t opSize) {
    // FIXME: this doesn't work - AND should not be part of 'emitData' (no need)
    // Move 'emitData' to base-class, no point in having a separate structure (right now atleast)
    emitFlags |= kEmitFlags::kEmitOpSize;
    EmitByte(opSize);
}

void EmitCodeStatement::EmitRegMode(uint8_t regMode) {
    emitFlags |= kEmitFlags::kEmitRegMode;
    EmitByte(regMode);
}

bool EmitCodeStatement::EmitInstrOperand(Context &context, vcpu::OperandDescription desc, vcpu::OperandSize opSize, ast::Expression::Ref operandExp) {

    // If this is an identifier - check if it is actually a constant and work out the actual value - recursively
    if (operandExp->Kind() == ast::NodeType::kIdentifier) {
        auto identifier = std::dynamic_pointer_cast<ast::Identifier>(operandExp);
        if (context.HasConstant(identifier->Symbol())) {
            auto constLiteral = context.GetConstant(identifier->Symbol());
            auto constExpression = EvaluateConstantExpression(context, constLiteral->Expression());
            return EmitInstrOperand(context, desc, opSize, constExpression);
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
                return EmitLabelAddress(context, std::dynamic_pointer_cast<ast::Identifier>(operandExp), opSize);
            } else {
                return EmitRelativeLabelAddress(context, std::dynamic_pointer_cast<ast::Identifier>(operandExp), opSize);
            }
            break;
        case ast::NodeType::kDeRefExpression :
            if (desc.features & vcpu::OperandDescriptionFlags::Addressing) {
                return EmitDereference(context, std::dynamic_pointer_cast<ast::DeReferenceExpression>(operandExp));
            }
            break;
        default :
            fmt::println(stderr, "Unsupported instr. operand type in AST");
            break;
    }
    return false;
}

bool EmitCodeStatement::EmitNumericLiteralForInstr(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {
    uint8_t regMode = 0; // no register
    regMode |= vcpu::AddressMode::Immediate;

    // Register|Mode = byte = RRRR | MMMM
    EmitRegMode(regMode);
    return EmitNumericLiteral(opSize, numLiteral);
}

bool EmitCodeStatement::EmitNumericLiteral(vcpu::OperandSize opSize, ast::NumericLiteral::Ref numLiteral) {

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

bool EmitCodeStatement::EmitRegisterLiteral(ast::RegisterLiteral::Ref regLiteral) {
    return EmitRegisterLiteralWithAddrMode(regLiteral, vcpu::AddressMode::Register);
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


bool EmitCodeStatement::EmitRegisterLiteralWithAddrMode(ast::RegisterLiteral::Ref regLiteral, uint8_t addrMode) {
    if (!regToIdx.contains(regLiteral->Symbol())) {
        fmt::println(stderr, "Illegal register: {}", regLiteral->Symbol());
        return false;
    }

    auto idxReg = regToIdx[regLiteral->Symbol()];
    // Register|Mode = byte = RRRR | MMMM
    auto regMode = (idxReg << 4) & 0xf0;
    regMode |= addrMode;

    EmitRegMode(regMode);
    return true;
}

// This is a bit hairy - to say the least
bool EmitCodeStatement::EmitDereference(Context &context, ast::DeReferenceExpression::Ref expression) {

    auto deref = EvaluateConstantExpression(context, expression->GetDeRefExp());

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

bool EmitCodeStatement::EmitLabelAddress(Context &context, ast::Identifier::Ref identifier, vcpu::OperandSize opSize) {
    uint8_t regMode = 0; // no register

    // This is an absolute jump
    regMode |= vcpu::AddressMode::Immediate;

    EmitOpSize(opSize);

    // Register|Mode = byte = RRRR | MMMM
    EmitRegMode(regMode);

    // This is how it should be done
    haveIdentifier = true;
    isRelative = false;
    symbol = identifier->Symbol();
    this->opSize = opSize;
    placeholderOffset = data.size();


    // Not needed
//    IdentifierAddressPlaceholder::Ref addressPlaceholder = std::make_shared<IdentifierAddressPlaceholder>();
//    addressPlaceholder->segment = context.Unit().GetActiveSegment();
//    addressPlaceholder->chunk = context.Unit().GetActiveSegment()->CurrentChunk();
//
//    // !!!!!!!!!!!!!!!
//    // FIXME: This must be resolved later!
//    // !!!!!!!!!!!!!
//    addressPlaceholder->address = 0; // static_cast<uint64_t>(context.Unit().GetCurrentWritePtr());
//    addressPlaceholder->ident = identifier->Symbol();
//    addressPlaceholder->isRelative = false;
//    // FIXME: This won't be needed!
//    context.AddAddressPlaceholder(addressPlaceholder);

    EmitLWord(0);   // placeholder
    return true;
}

bool EmitCodeStatement::EmitRelativeLabelAddress(Context &context, ast::Identifier::Ref identifier, vcpu::OperandSize opSize) {
    uint8_t regMode = 0; // no register

    // This a relative jump
    regMode |= vcpu::AddressMode::Immediate;

    IdentifierAddressPlaceholder::Ref addressPlaceholder = std::make_shared<IdentifierAddressPlaceholder>();
    addressPlaceholder->segment = context.Unit().GetActiveSegment();
    addressPlaceholder->chunk = context.Unit().GetActiveSegment()->CurrentChunk();

    if (opSize == vcpu::OperandSize::Long) {
        // no op.size were given
        // FIXME: Compute distance - how?

        if (!context.HasIdentifierAddress(identifier->Symbol())) {
            fmt::println(stderr, "Compiler, Identifier not found - can't compute jump length...");
            return false;
        } else {
            fmt::println("Compiler, warning - realtive address instr. detected without operand size specification - trying to deduce");
            auto idAddress = context.GetIdentifierAddress(identifier->Symbol()); // identifierAddresses[identifier->Symbol()];
            // We are ahead...
            auto dist = (context.Unit().GetCurrentWritePtr() - idAddress.address);
            if (dist < 255) {
                fmt::println("Compiler, warning - changing from unspecfied to byte (dist={})", dist);
                opSize = vcpu::OperandSize::Byte;
            } else if (dist < 65545) {
                fmt::println("Compiler, warning - changing from unspecfied to word (dist={})", dist);
                opSize = vcpu::OperandSize::Word;
            } else {
                fmt::println("Compiler, warning - changing from unspecfied to dword (dist={})", dist);
                opSize = vcpu::OperandSize::DWord;
            }
        }
    }

    // Ok, we have changed it - so if we deferred it - let's write it out now...
    // Note: we can NOT write anything before this has been done...
    EmitOpSize(opSize);

    // Register|Mode = byte = RRRR | MMMM
    EmitRegMode(regMode);

    haveIdentifier = true;
    symbol = identifier->Symbol();
    isRelative = true;
    this->opSize = opSize;
    // not sure if needed
    //ofsRelative = static_cast<uint64_t>(emitData.data.size()) + vcpu::ByteSizeOfOperandSize(opSize);
    placeholderOffset = data.size();



//    // !!!!!!!!!!!!!!!
//    // FIXME: Should be a resolve point here!
//    // !!!!!!!!!!!!!
//    addressPlaceholder->address = 0;
//    addressPlaceholder->ident = identifier->Symbol();
//    addressPlaceholder->isRelative = true;
//    addressPlaceholder->opSize = opSize;
//
//    // We are always relative to the complete instruction
//    // Note: could have been relative to the start of the instr. but that would require a state (or caching of the IP) in the cpu
//    //addressPlaceholder.ofsRelative = static_cast<uint64_t>(context.Unit().GetCurrentWritePtr()) + vcpu::ByteSizeOfOperandSize(opSize);
//    addressPlaceholder->ofsRelative = static_cast<uint64_t>(emitData.data.size()) + vcpu::ByteSizeOfOperandSize(opSize);
//
//    // FIXME: this won't be needed!
//    context.AddAddressPlaceholder(addressPlaceholder);
//    //addressPlaceholders.push_back(addressPlaceholder);

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
            fmt::println(stderr, "Compiler, Relative Addressing but Absolute value - see: 'EmitRelativeAddress'");
            return false;
    }
    return true;
}

//
// Evaluate a constant expression...
//
ast::Literal::Ref EmitCodeStatement::EvaluateConstantExpression(Context &context, ast::Expression::Ref expression) {
    switch (expression->Kind()) {
        case ast::NodeType::kNumericLiteral :
            return std::dynamic_pointer_cast<ast::Literal>(expression);
        case ast::NodeType::kBinaryExpression :
            return EvaluateBinaryExpression(context, std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
        case ast::NodeType::kDeRefExpression : {
            // If the expression starts with '(' this will be marked as a de-reference expression
            // let's just forward the internal expression in that case and hope for the best...
            auto deRefExp = std::dynamic_pointer_cast<ast::DeReferenceExpression>(expression);
            return EvaluateConstantExpression(context, deRefExp->GetDeRefExp());
        }
        case ast::NodeType::kRegisterLiteral :
            return std::dynamic_pointer_cast<ast::Literal>(expression);
        case ast::NodeType::kIdentifier : {
            auto identifier = std::dynamic_pointer_cast<ast::Identifier>(expression);
            if (context.HasConstant(identifier->Symbol())) {
                auto constLiteral = context.GetConstant(identifier->Symbol());
                return EvaluateConstantExpression(context, constLiteral->Expression());
            }
            return nullptr;
        }
        case ast::NodeType::kMemberExpression :
            fmt::println("Compiler, member expression");
            return EvaluateMemberExpression(context, std::dynamic_pointer_cast<ast::MemberExpression>(expression));
            break;
        case ast::NodeType::kStringLiteral :
            fmt::println(stderr, "Compiler, string literals as constants are not yet supported!");
            exit(1);
        default :
            fmt::println(stderr, "Compiler, Unsupported node type: {} in constant expression", ast::NodeTypeToString(expression->Kind()));
            exit(1);
    }
    return {};
}

ast::Literal::Ref EmitCodeStatement::EvaluateBinaryExpression(Context &context, ast::BinaryExpression::Ref expression) {
    auto lhs = EvaluateConstantExpression(context, expression->Left());
    auto rhs = EvaluateConstantExpression(context, expression->Right());
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

ast::Literal::Ref EmitCodeStatement::EvaluateMemberExpression(Context &context, ast::MemberExpression::Ref expression) {
    auto &ident = expression->Ident();
    // Try find the structure matching the name
    if (!context.HasStructDefinintion(ident->Symbol())) {
        fmt::println(stderr, "Compiler, Unable to find struct '{}'", ident->Symbol());
        return nullptr;
    }

    auto &structDef = context.GetStructDefinitionFromTypeName(ident->Symbol());

    // Make sure out member is an identifier (this should always be the case - but ergo - it isn't)
    if (expression->Member()->Kind() != ast::NodeType::kIdentifier) {
        fmt::println(stderr, "Compiler, expected identifier in member expression");
        return nullptr;
    }

    // Now, find the struct member
    auto identMember = std::dynamic_pointer_cast<ast::Identifier>(expression->Member());
    auto memberName = identMember->Symbol();
    if (!structDef.HasMember(identMember->Symbol())) {
        fmt::println(stderr, "Compiler, no member in struct {} named {}", structDef.ident, identMember->Symbol());
        return nullptr;
    }
    auto &structMember = structDef.GetMember(identMember->Symbol());

    // Here we simply create a numeric literal of the offset to the struct member
    // Now, this is not quite the best way to do this - since we would like to verify/check
    // - size of operand matches the size of the element
    // - alignment

    return ast::NumericLiteral::Create(structMember.offset);
}
