//
// Created by gnilk on 10.03.2024.
//

// This is very much tied to the current instruction set we are assembling
// Can we make this more 'generic'?


#include "CompileUnit.h"
#include "Identifiers.h"
#include "StmtEmitter.h"
#include "InstructionSet.h"

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
        ref = std::make_shared<EmitStructStatement>();
    } else if (IsCodeStatement(statement->Kind())) {
        ref = std::make_shared<EmitCodeStatement>();
    } else if (statement->Kind() == ast::NodeType::kCommentStatement){
        ref = std::make_shared<EmitCommentStatement>();
    } else if (statement->Kind() == ast::NodeType::kExportStatement) {
        ref = std::make_shared<EmitExportStatement>();
    } else {
        fmt::println("StateEmitter - {} - not handled", ast::NodeTypeToString(statement->Kind()));
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
            (nodeType == ast::NodeType::kConstLiteral) ||
            (nodeType == ast::NodeType::kStructLiteral)) {
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

bool EmitMetaStatement::Process(CompileUnit &context) {
    return ProcessMetaStatement(context, std::dynamic_pointer_cast<ast::MetaStatement>(statement));
}

bool EmitMetaStatement::ProcessMetaStatement(CompileUnit &context, ast::MetaStatement::Ref stmt) {

    if (stmt->Symbol() == "org") {
        auto arg = stmt->Argument();
        if (arg == nullptr) {
            fmt::println(stderr, "Compiler, .org directive requires expression");
            return false;
        }
        if (arg->Kind() != ast::NodeType::kNumericLiteral) {
            fmt::println(stderr, "Compiler, .org expects numeric expression");
            return false;
        }
        auto numArg = std::dynamic_pointer_cast<ast::NumericLiteral>(arg);

        origin = numArg->Value();
        isOrigin = true;
        return true;
    }

    // Not origin - we are segment, so just store this...
    segmentName = stmt->Symbol();
    return true;
}

bool EmitMetaStatement::Finalize(gnilk::assembler::CompileUnit &context) {
    if (!isOrigin) {
        return FinalizeSegment(context);
    }

    // Create a new chunk if none-exists or if we are not empty..
    auto currentSegment = context.GetActiveSegment();
    if ((currentSegment->CurrentChunk() == nullptr) || (!currentSegment->CurrentChunk()->Empty())) {
        currentSegment->CreateChunk(origin);
        return true;
    }

    // This is an empty - implicitly created - just change the origin address..
    currentSegment->CurrentChunk()->SetLoadAddress(origin);
    return true;
}

bool EmitMetaStatement::FinalizeSegment(CompileUnit &context) {
    auto [ok, type] = Segment::TypeFromString(segmentName);
    if (!ok) {
        fmt::println(stderr, "Compiler, unsupported segment type '{}', use: .code/.text or .data", segmentName);
        return false;
    }
    context.CreateEmptySegment(type);
    return true;
}

EmitExportStatement::EmitExportStatement() {
    emitType = kEmitType::kExport;
}

bool EmitExportStatement::Process(CompileUnit &context) {
    auto expStatement = std::dynamic_pointer_cast<ast::ExportStatement>(statement);
    identifier = expStatement->Identifier();
    context.AddExport(identifier);
    return true;
}

bool EmitExportStatement::Finalize(CompileUnit &context) {
    if (context.HasExport(identifier)) {
        auto expIdent = context.GetExport(identifier);
        // We had this export already - but not origIdentifier yet - so this is most likely a forward declaration
        // i.e. we first call a unit that use the export, but we have not yet compiled the unit which exports it
        // the linker will check this later - the compiler can't...
        if (expIdent->origIdentifier == nullptr) {
            // Update the name - it is never set through forward declaration...
            expIdent->name = identifier;
            return true;
        }
        fmt::println(stderr, "Compiler, symbol {} already exported", identifier);
    }

    context.AddExport(identifier);
    return true;
}

EmitCommentStatement::EmitCommentStatement() {
    emitType = kEmitType::kComment;
}

bool EmitCommentStatement::Process(gnilk::assembler::CompileUnit &context) {
    auto cmtStatement = std::dynamic_pointer_cast<ast::LineComment>(statement);
    text = cmtStatement->Text();
    return true;
}

bool EmitCommentStatement::Finalize(gnilk::assembler::CompileUnit &context) {
    return true;
}

///////////////////
//
// Data Emitters
//
EmitDataStatement::EmitDataStatement() {
    emitType = kEmitType::kData;
}

bool EmitDataStatement::Process(gnilk::assembler::CompileUnit &context) {
    if (statement->Kind() == ast::NodeType::kConstLiteral) {
        return ProcessConstLiteral(context, std::dynamic_pointer_cast<ast::ConstLiteral>(statement));
    } else if (statement->Kind() == ast::NodeType::kStructLiteral) {
        return ProcessStructLiteral(context, std::dynamic_pointer_cast<ast::StructLiteral>(statement));
    }
    return ProcessArrayLiteral(context, std::dynamic_pointer_cast<ast::ArrayLiteral>(statement));
}

bool EmitDataStatement::Finalize(gnilk::assembler::CompileUnit &context) {
    // Data statement is also 'const' declaration - they have no data, so check if we are empty before executing
    // write. As write returns num-bytes written, which is zero for const declarations...
    if (!data.empty() && !context.Write(data)) {
        return false;
    }
    return true;
}

bool EmitDataStatement::ProcessArrayLiteral(gnilk::assembler::CompileUnit &context, ast::ArrayLiteral::Ref stmt) {
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

bool EmitDataStatement::ProcessConstLiteral(CompileUnit &context, ast::ConstLiteral::Ref stmt) {

    if (context.HasConstant(stmt->Ident())) {
        fmt::println(stderr, "Compiler, const with name '{}' already defined", stmt->Ident());
        return false;
    }
    context.AddConstant(stmt->Ident(), stmt);
    return true;
}

bool EmitDataStatement::ProcessStructLiteral(CompileUnit &context, ast::StructLiteral::Ref stmt) {

    size_t szExpected = 0;
    if (!context.HasStructDefinintion(stmt->StructTypeName())) {
        fmt::println(stderr, "Compiler, undefined struct type '{}'", stmt->StructTypeName());
        return false;
    }

    auto structDef = context.GetStructDefinitionFromTypeName(stmt->StructTypeName());
    if (structDef == nullptr) {
        fmt::println(stderr, "Compiler, critical error; no struct with type name={}", stmt->StructTypeName());
        return false;
    }
    szExpected = structDef->byteSize;

    // Reserve the amount of bytes we need
    data.reserve(szExpected);

    //auto writeStart = context.GetCurrentWriteAddress();
    size_t szOffset = 0;
    size_t idxMember = 0;
    for (auto &m : stmt->Members()) {
        if (idxMember > structDef->NumMembers()) {
            fmt::println(stderr,"Compiler, declaring too many members");
            break;
        }
        auto &structDefMember = structDef->GetMemberAt(idxMember);
        fmt::println("Assigning to struct member '{}'", structDefMember.ident);

        auto memberEmitter = EmitStatementBase::Create(m);
        if (memberEmitter == nullptr) {
            fmt::println(stderr, "Compiler, Member is statement type '{}' not a supported type for struct member declarations", ast::NodeTypeToString(m->Kind()));
            return false;
        }
        if (memberEmitter->Kind() != kEmitType::kData) {
            fmt::println(stderr, "Compiler, Struct member is no data statement!");
            return false;
        }
        auto member = std::dynamic_pointer_cast<EmitDataStatement>(memberEmitter);
        if (member == nullptr) {
            // wrong type?
            fmt::println(stderr, "Compiler, struct member is null!!!");
            return false;
        }

        if (!member->Process(context)) {
            fmt::println(stderr, "Compiler, Struct member process failed!");
            return false;
        }

        //structMemberStatements.push_back(memberEmitter);
        auto &memberData = member->Data();
        // TO-DO: Support struct initialization through .<name> syntax (as with C/C++)
        // structDef.GetMember(member->Symbol()).offset

        if ((szOffset + memberData.size()) > data.capacity()) {
            // Overflow is generally not a problem when declaring a struct..
            // C allows last member to overflow as an empty array declaration
            // we allow every member (as we don't really track which member)
        }

        data.insert(data.begin() + szOffset, memberData.begin(), memberData.end());
        // Advance forward..
        // TO-DO: IF we do: structDefMember.byteSize; we would advance properly to the next element even
        //        if a member is defined as word and declared using byte...
        szOffset += memberData.size();
        idxMember++;

    }
    fmt::println("struct literal - bytes generated {} - bytes expected {}", data.size(), szExpected);
    auto nBytesGenerated =  data.size();

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


///////////
EmitIdentifierStatement::EmitIdentifierStatement() {
    emitType = kEmitType::kIdentifier;
}

bool EmitIdentifierStatement::Process(CompileUnit &context) {
    return ProcessIdentifier(context, std::dynamic_pointer_cast<ast::Identifier>(statement));
}

bool EmitIdentifierStatement::Finalize(CompileUnit &context) {
    // Make sure we have somewhere to place our data
    // This in case someone does '.code' but not .org statement
    context.EnsureChunk();

    // The address is the current write point...
    if (!context.HasIdentifier(symbol)) {
        fmt::println(stderr, "Compiler, No such identifier '{}'", symbol);
        return false;
    }
    auto ident = context.GetIdentifier(symbol);
    ident->segment = context.GetActiveSegment();
    if (ident->segment == nullptr) {
        fmt::println(stderr, "Compiler, no active segment!");
        return false;
    }
    ident->chunk = context.GetActiveSegment()->CurrentChunk();
    if (ident->chunk == nullptr) {
//        fmt::println(stderr, "Compiler, no active 'chunk' in segment {}", ident->segment->Name());
//        // FIXME: Is this path even a good thing??
//
//        // Create a new chunk at the exact write address
//        auto loadAddress = context.GetCurrentWriteAddress();
//        ident->segment->CreateChunk(loadAddress);
//        ident->chunk = context.GetActiveSegment()->CurrentChunk();
        return false;
    }
    // Set the absolute address
    ident->absoluteAddress = context.GetCurrentWriteAddress();
    ident->relativeAddress = context.GetCurrentRelativeWriteAddress();

    fmt::println("EmitIdentifier, {} @ {}", symbol, ident->absoluteAddress);
    // Update the export - this way we create a shortcut when the linker kicks in and wants to write out the
    // symbol table for all exports...
    if (context.HasExport(symbol)) {
        auto identExport = context.GetExport(symbol);
        identExport->absoluteAddress = ident->absoluteAddress;
        identExport->relativeAddress = ident->relativeAddress;
    }
    return true;
}


bool EmitIdentifierStatement::ProcessIdentifier(CompileUnit &context, ast::Identifier::Ref identifier) {
    uint64_t ipNow = GetCurrentAddress();

    // Need to register here so we can bail early
    if (context.HasIdentifier(identifier->Symbol()) && !context.HasExport(identifier->Symbol())) {
        fmt::println(stderr,"Identifier {} already in use - can't use duplicate identifiers!", identifier->Symbol());
        return false;
    }

    symbol = identifier->Symbol();
//    address = 0;


    // Actually we just need to make sure it is registered...
    // Not quite sure all of this is needed just yet..
    // As we now have a more linear view of the meta data..
    Identifier idAddress = {};

    context.AddIdentifier(identifier->Symbol(), idAddress);
    return true;
}

EmitStructStatement::EmitStructStatement() : EmitStatementBase() {
    emitType = kEmitType::kStruct;
}
bool EmitStructStatement::Process(gnilk::assembler::CompileUnit &context) {
    return ProcessStructStatement(context, std::dynamic_pointer_cast<ast::StructStatement>(statement));
}
bool EmitStructStatement::Finalize(gnilk::assembler::CompileUnit &context) {
    return true;
}
bool EmitStructStatement::ProcessStructStatement(CompileUnit &context, ast::StructStatement::Ref stmt) {
    auto &members = stmt->Declarations();
    size_t nBytes = 0;

    for(auto &m : members) {
        size_t nToAdd = 0;
        if (m->Kind() == ast::NodeType::kReservationStatementNative) {
            nToAdd += ProcessNativeMember(context, m, nBytes);
        } else {
            nToAdd += ProcessCustomMember(context, m, nBytes);
        }

        if (!nToAdd) {
            return false;
        }
        nBytes += nToAdd;
    }

    StructDefinition structDefinition  = {
            .ident = stmt->Name(),
            .byteSize = nBytes,
            .members = std::move(structMembers),
    };
    context.AddStructDefinition(structDefinition);

    return true;
}

size_t EmitStructStatement::ProcessNativeMember(CompileUnit &context, ast::Statement::Ref stmt, size_t currentOffset) {
    auto reservation = std::dynamic_pointer_cast<ast::ReservationStatementNativeType>(stmt);

    auto elemLiteral = EvaluateConstantExpression(context, reservation->NumElements());
    if (elemLiteral->Kind() != ast::NodeType::kNumericLiteral) {
        fmt::println(stderr, "Compiler, reservation expression did not yield a numerical result!");
        return false;
    }
    auto numElem = std::dynamic_pointer_cast<ast::NumericLiteral>(elemLiteral);

    //
    // FIXME: In case this is struct reference we need do recursively add more
    //        We should probably NOT do this - even it would work...

    auto szPerElem = reservation->ElementByteSize();

    //auto szPerElem = vcpu::ByteSizeOfOperandSize(reservation->OperandSize());

    StructMember member;
    member.ident = reservation->Identifier()->Symbol();
    member.offset = currentOffset;
    member.byteSize = numElem->Value() * szPerElem;
    // Backup...
    member.declarationStatement = stmt;

    structMembers.push_back(member);

    return numElem->Value() * szPerElem;
}
size_t EmitStructStatement::ProcessCustomMember(CompileUnit &context, ast::Statement::Ref stmt, size_t currentOffset) {
    auto reservation = std::dynamic_pointer_cast<ast::ReservationStatementCustomType>(stmt);
    if (reservation->CustomType()->Kind() != ast::NodeType::kStructStatement) {
        fmt::println(stderr, "Compiler, emit struct statement, not a struct!");
        return 0;
    }
    auto structStmt = std::dynamic_pointer_cast<ast::StructStatement>(reservation->CustomType());
    if (structStmt == nullptr) {
        fmt::println(stderr,"Compiler, internal compiler error in EmitStructStatement::ProcessCustomMember");
        exit(1);
    }
    size_t nBytes = 0;
    for (auto &m : structStmt->Declarations()) {
        if (m->Kind() == ast::NodeType::kReservationStatementNative) {
            nBytes += ProcessNativeMember(context, m, currentOffset);
        } else if (m->Kind() == ast::NodeType::kReservationStatementCustom) {
            nBytes += ProcessCustomMember(context, m, currentOffset);
        } else {
            fmt::println(stderr, "Compiler, emit struct statement - not a valid node type!");
        }
        currentOffset += nBytes;
    }
    return nBytes;
}



EmitCodeStatement::EmitCodeStatement() {
    emitType = kEmitType::kCode;
}

bool EmitCodeStatement::Process(gnilk::assembler::CompileUnit &context) {
    postEmitters.clear();

    if(statement->Kind() == ast::NodeType::kNoOpInstrStatement) {
        return ProcessNoOpInstrStmt(std::dynamic_pointer_cast<ast::NoOpInstrStatment>(statement));
    } else if (statement->Kind() == ast::NodeType::kOneOpInstrStatement) {
        return ProcessOneOpInstrStmt(context, std::dynamic_pointer_cast<ast::OneOpInstrStatment>(statement));
    } else if (statement->Kind() == ast::NodeType::kTwoOpInstrStatement) {
        return ProcessTwoOpInstrStmt(context, std::dynamic_pointer_cast<ast::TwoOpInstrStatment>(statement));
    }
    return false;
}

bool EmitCodeStatement::Finalize(gnilk::assembler::CompileUnit &context) {

    if (temp_isDeferredEmitter) {
        data.clear();
        if (!Process(context)) {
            return false;
        }
    }

    if (haveIdentifier) {
        auto currentSegment = context.GetActiveSegment();
        if (currentSegment == nullptr) {
            fmt::println(stderr, "Compiler, no segment - can't emit code statement");
            return false;
        }
        Identifier::Ref identifier = nullptr;

        if (context.HasIdentifier(symbol)) {
            // Local symbol, this symbol is added to the identifier list during Processing of the statement
            identifier = context.GetIdentifier(symbol);

        } else if (context.HasExport(symbol) && context.IsExportLocal(symbol)) {
            // In case this unit holds the export - we treat it as a regular identifier
            identifier = context.GetIdentifier(symbol);

        } else {
            // previously unknown symbol, we assume this is an implicit forward declaration...
            // this is actually not an export - this is an import for this module!
            identifier = context.AddImport(symbol);
        }
        auto relocatedPlaceholder = placeholderAddress + currentSegment->CurrentChunk()->GetRelativeWriteAddress();
        // FIXME: Not sure we should add the current write address here...
        identifier->resolvePoints.push_back({
                                                    .segment = currentSegment,
                                                    .chunk = currentSegment->CurrentChunk(),
                                                    .opSize = opSize,
                                                    .isRelative = isRelative,
                                                    .placeholderAddress =  relocatedPlaceholder,
                                                    //.placeholderAddress =  context.GetCurrentWriteAddress() + placeholderAddress,
                                            });


    }

    if (!context.Write(data)) {
        return false;
    }

    return true;
}
void EmitCodeStatement::AddPostEmitter(PostEmitOpData emitter) {
    postEmitters.push_back(emitter);
}

bool EmitCodeStatement::RunPostEmitters() {
    for(auto emitter : postEmitters) {
        if (!emitter()) {
            return false;
        }
    }
    return true;
}

bool EmitCodeStatement::ProcessNoOpInstrStmt(ast::NoOpInstrStatment::Ref stmt) {
    if (!EmitOpCodeForSymbol(stmt->Symbol())) {
        return false;
    }
    return true;
}


// FIXME: InstructionSet dependent?
bool EmitCodeStatement::ProcessOneOpInstrStmt(CompileUnit &context, ast::OneOpInstrStatment::Ref stmt) {
    if (!EmitOpCodeForSymbol(stmt->Symbol())) {
        return false;
    }

    auto opSize = stmt->OpSize();

    //
    auto opSizeWritePoint = data.size(); // Save where we are

    auto &instrSet = vcpu::InstructionSetManager::Instance().GetInstructionSet();
    auto opClass = *instrSet.GetDefinition().GetOperandFromStr(stmt->Symbol());
    auto opDesc = *instrSet.GetDefinition().GetOpDescFromClass(opClass);


    if (!EmitInstrOperand(context, opDesc, opSize, stmt->Operand())) {
        return false;
    }

    // Was opsize genereated - in case not - insert it...
    if (!(emitFlags & kEmitFlags::kEmitOpSize)) {
        data.insert(data.begin() + opSizeWritePoint, opSize);
    }

    // Now emit any left-overs belonging to the operands
    RunPostEmitters();

    return true;
}

// FIXME: InstructionSet dependent?
bool EmitCodeStatement::ProcessTwoOpInstrStmt(CompileUnit &context, ast::TwoOpInstrStatment::Ref twoOpInstr) {
    if (twoOpInstr->Symbol() == "lea") {
        int breakme = 1;
    }

    if (!EmitOpCodeForSymbol(twoOpInstr->Symbol())) {
        return false;
    }

    // Fetch the instruction set..
    auto &instrSet = vcpu::InstructionSetManager::Instance().GetInstructionSet();


    // FIXME: This can't be correct in case we have to recalibrate the opsize [will we ever need that for a two-op instr?]
    auto opSize = twoOpInstr->OpSize();
    uint8_t opSizeAndFamilyCode = static_cast<uint8_t>(opSize);
    //
    //  We should have the instr.set encode this...
    //
    opSizeAndFamilyCode |= static_cast<uint8_t>(twoOpInstr->OpFamily()) << 4;

//    auto opSizeAndFamilyCode = instrSet.GetDefinition().EncodeOpSizeAndFamily(twoOpInstr->OpSize(), twoOpInstr->OpFamily());

    auto opClass = *instrSet.GetDefinition().GetOperandFromStr(twoOpInstr->Symbol());
    auto opDesc = *instrSet.GetDefinition().GetOpDescFromClass(opClass);

    // Save the write point..
    auto opSizeWritePoint = data.size();
    // This is temporary (might be changed)
    // FIXME: Verify this - not sure this is correct..
    EmitOpSize(opSizeAndFamilyCode);

    // This can't output opSize..
    if (!EmitInstrOperand(context, opDesc, opSize, twoOpInstr->Dst())) {
        return false;
    }

    // This can output opSize..
    if (!EmitInstrOperand(context, opDesc, opSize, twoOpInstr->Src())) {
        return false;
    }

    // Now run any post emitters
    RunPostEmitters();

    // If we didn't write the operand size and family, let's do it now...
    if (!(emitFlags & kEmitFlags::kEmitOpSize)) {
        // FIXME: This branch is never hit - need unit test to specifically stress this (if even possible)
        data.insert(data.begin() + opSizeWritePoint, opSize);
    }

    return true;
}

// FIXME: Instruction set dependent
bool EmitCodeStatement::EmitOpCodeForSymbol(const std::string &symbol) {

    auto &instrSet = vcpu::InstructionSetManager::Instance().GetInstructionSet();
    auto opcode = instrSet.GetDefinition().GetOperandFromStr(symbol);
    if (!opcode.has_value()) {
        fmt::println(stderr, "Unknown/Unsupported symbol: {}", symbol);
        return false;
    }
    emitFlags |= kEmitFlags::kEmitOpCode;
    return EmitByte(*opcode);
}

void EmitCodeStatement::EmitOpSize(uint8_t opSize) {
    emitFlags |= kEmitFlags::kEmitOpSize;
    EmitByte(opSize);
}

void EmitCodeStatement::EmitRegMode(uint8_t regMode) {
    emitFlags |= kEmitFlags::kEmitRegMode;
    EmitByte(regMode);
}


bool EmitCodeStatement::EmitInstrOperand(CompileUnit &context, const vcpu::OperandDescriptionBase &desc, vcpu::OperandSize opSize, ast::Expression::Ref operandExp) {

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
            if (desc.features & vcpu::OperandFeatureFlags::kFeature_Immediate) {
                return EmitNumericLiteralForInstr(opSize, std::dynamic_pointer_cast<ast::NumericLiteral>(operandExp));
            }
            fmt::println(stderr, "Instruction Operand does not support immediate");
            break;
        case ast::NodeType::kRegisterLiteral :
            if (desc.features & vcpu::OperandFeatureFlags::kFeature_AnyRegister) {
                return EmitRegisterLiteral(std::dynamic_pointer_cast<ast::RegisterLiteral>(operandExp));
            }
            fmt::println(stderr, "Instruction Operand does not support register");
            break;
        case ast::NodeType::kIdentifier :
            // After statement parsing is complete we will change all place-holders
            if ((desc.features & vcpu::OperandFeatureFlags::kFeature_Addressing) && (opSize == vcpu::OperandSize::Long)) {
                return EmitLabelAddress(context, desc, std::dynamic_pointer_cast<ast::Identifier>(operandExp), opSize);
            } else {
                return EmitRelativeLabelAddress(context, desc, std::dynamic_pointer_cast<ast::Identifier>(operandExp), opSize);
            }
            break;
        case ast::NodeType::kDeRefExpression :
            if (desc.features & vcpu::OperandFeatureFlags::kFeature_Addressing) {
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
    AddPostEmitter([this, opSize, numLiteral]() -> bool {
            return EmitNumericLiteral(opSize, numLiteral);
        });
    //return EmitNumericLiteral(opSize, numLiteral);
    return true;
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

// FIXME: Instruction set dependent
//   Note: these are 'raw' values - in order to have them in the REGMODE byte of an instr. they must be shifted
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
bool EmitCodeStatement::EmitDereference(CompileUnit &context, ast::DeReferenceExpression::Ref expression) {

    // Note: Dereference can be to anything - a non-constant identifier..
    auto deRefExp = expression->GetDeRefExp();
    if (deRefExp->Kind() == ast::NodeType::kIdentifier) {
        fmt::println(stderr, "Compiler, dereferencing identifiers are not supported");
        return false;
    }


    auto deref = EvaluateConstantExpression(context, expression->GetDeRefExp());
    if (deref == nullptr) {
        fmt::println(stderr, "Warning, dereference is null - deferring this emitter to later to check if it works");
        temp_isDeferredEmitter = true;
        return true;
    }

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
            AddPostEmitter([this, relativeRegLiteral]()->bool{
                return EmitRegisterLiteralWithAddrMode(std::dynamic_pointer_cast<ast::RegisterLiteral>(relativeRegLiteral->RelativeExpression()), 0);
            });
            return true;
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
            AddPostEmitter([this, relLiteral]()->bool{return EmitByte(relLiteral->Value());});
            //return EmitByte(relLiteral->Value());
            return true;
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

            AddPostEmitter([this, relRegShift, shiftNum]()->bool{
                return EmitRegisterLiteralWithAddrMode(relRegShift->BaseRegister(), shiftNum);
            });
            // ?????
//            addrMode = shiftNum;
//            if (!EmitRegisterLiteralWithAddrMode(relRegShift->BaseRegister(), addrMode)) {
//                return false;
//            }
            return true;
        }
    }

    fmt::println(stderr, "Compiler, Unsupported dereference construct!");
    return false;
}

// FIXME: need pointer/ref to current instr. set
bool EmitCodeStatement::EmitLabelAddress(CompileUnit &context, const vcpu::OperandDescriptionBase &desc, ast::Identifier::Ref identifier, vcpu::OperandSize opSize) {
    uint8_t regMode = 0; // no register

    // This is an absolute jump
    // For relative branches this should be 'Immediate' for absolute addresses this should be absolute...

    // FIXME: How to distinguish when/where we should have immediate or absolute
    // like: 'call label' <- should be immediate (i.e. the value is placed here and should be used as a fixed jump point)
    //       'cmp.l label, 0'  <- should be absolute (i.e. the absolute value from address of label should be read)

    // FIXME: need pointer/ref to current instr. set
    //        -> Replace 'IsFeatureSupported()'
    if (desc.features & vcpu::OperandFeatureFlags::kFeature_Branching) {
        regMode |= vcpu::AddressMode::Immediate;
    } else {
        regMode |= vcpu::AddressMode::Absolute;
    }

    // problem?
    //   We need to understand if this is a single-op instruction or two-op instruction
    //   in case of two-op instr. we can't really do this...
    //
    // FIXME: Try to create a unit test stressing this problem - with mixing operand size (not sure why this should happen)
    //
    if (!(emitFlags & kEmitOpSize)) {
        EmitOpSize(opSize);
    }

    // Register|Mode = byte = RRRR | MMMM
    EmitRegMode(regMode);

    // This is how it should be done
    haveIdentifier = true;
    isRelative = false;
    symbol = identifier->Symbol();
    this->opSize = opSize;

    AddPostEmitter([this]()->bool{
            placeholderAddress = data.size();       // Set address to the location within the instruction
            return EmitLWord(0);   // placeholder
        });
    return true;
}

// FIXME: This must be done in the post-emit stage, which is a problem - since it will be hard to compute the actual jump size...
//        What I would need to is to 'estimate' or 'guess' the size of the jump - and the recompute everything if it doesn't work out...
bool EmitCodeStatement::EmitRelativeLabelAddress(CompileUnit &context, const vcpu::OperandDescriptionBase &desc, ast::Identifier::Ref identifier, vcpu::OperandSize opSize) {
    uint8_t regMode = 0; // no register

    // This a relative jump
    regMode |= vcpu::AddressMode::Immediate;

    if (opSize == vcpu::OperandSize::Long) {

        // Let's see if this is a known identifier - in case it isn't we defer this call 'til later (post)
        if (!context.HasIdentifier(identifier->Symbol())) {
            // If we are in the deferred emitter, this is bad!
            if (temp_isDeferredEmitter) {
                fmt::println(stderr, "Compiler, warning identifier '{}' not found - can't compute jump length...", identifier->Symbol());
                return false;
            }
            temp_isDeferredEmitter = true;
            return true;
        }

        // no op.size were given, let's compute distance

        fmt::println("Compiler, warning - relative address instr. detected without operand size specification - trying to deduce");
        auto idAddress = context.GetIdentifier(identifier->Symbol()); // identifierAddresses[identifier->Symbol()];
        // We are ahead...
        auto dist = (context.GetCurrentWriteAddress() - idAddress->absoluteAddress);
        if (dist < 255) {
            fmt::println("Compiler, warning - changing from unspecified to byte (dist={})", dist);
            opSize = vcpu::OperandSize::Byte;
        } else if (dist < 65545) {
            fmt::println("Compiler, warning - changing from unspecified to word (dist={})", dist);
            opSize = vcpu::OperandSize::Word;
        } else {
            fmt::println("Compiler, warning - changing from unspecified to dword (dist={})", dist);
            opSize = vcpu::OperandSize::DWord;
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
    AddPostEmitter([this,opSize](){
        placeholderAddress = data.size();

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
    });
    return true;
}

//
// Evaluate a constant expression...
//
ast::Literal::Ref EmitStatementBase::EvaluateConstantExpression(CompileUnit &context, ast::Expression::Ref expression) {
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
                if (!context.HasConstant(identifier->Symbol())) {
                    return nullptr;
                }
                auto constLiteral = context.GetConstant(identifier->Symbol());
                return EvaluateConstantExpression(context, constLiteral->Expression());
            }
        case ast::NodeType::kMemberExpression :
            fmt::println("Compiler, member expression");
            return EvaluateMemberExpression(context, std::dynamic_pointer_cast<ast::MemberExpression>(expression));
        case ast::NodeType::kStringLiteral :
            fmt::println(stderr, "Compiler, string literals as constants are not yet supported!");
            exit(1);
        default :
            fmt::println(stderr, "Compiler, Unsupported node type: {} in constant expression", ast::NodeTypeToString(expression->Kind()));
            exit(1);
    }
    return {};
}

ast::Literal::Ref EmitStatementBase::EvaluateBinaryExpression(CompileUnit &context, ast::BinaryExpression::Ref expression) {
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

ast::Literal::Ref EmitStatementBase::EvaluateMemberExpression(CompileUnit &context, ast::MemberExpression::Ref expression) {
    auto &ident = expression->Ident();
    // Try find the structure matching the name
    if (!context.HasStructDefinintion(ident->Symbol())) {
        fmt::println(stderr, "Compiler, Unable to find struct '{}'", ident->Symbol());
        return nullptr;
    }

    auto structDef = context.GetStructDefinitionFromTypeName(ident->Symbol());

    // Make sure out member is an identifier (this should always be the case - but ergo - it isn't)
    if (expression->Member()->Kind() != ast::NodeType::kIdentifier) {
        fmt::println(stderr, "Compiler, expected identifier in member expression");
        return nullptr;
    }

    // Now, find the struct member
    auto identMember = std::dynamic_pointer_cast<ast::Identifier>(expression->Member());
    auto memberName = identMember->Symbol();
    if (!structDef->HasMember(identMember->Symbol())) {
        fmt::println(stderr, "Compiler, no member in struct {} named {}", structDef->ident, identMember->Symbol());
        return nullptr;
    }
    auto &structMember = structDef->GetMember(identMember->Symbol());

    // Here we simply create a numeric literal of the offset to the struct member
    // Now, this is not quite the best way to do this - since we would like to verify/check
    // - size of operand matches the size of the element
    // - alignment

    return ast::NumericLiteral::Create(structMember.offset);
}
