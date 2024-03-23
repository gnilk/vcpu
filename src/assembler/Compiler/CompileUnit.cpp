//
// Created by gnilk on 20.12.23.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include "CompileUnit.h"
#include "Compiler/Context.h"

using namespace gnilk;
using namespace gnilk::assembler;

void CompileUnit::Clear() {
    emitStatements.clear();
}

// Process the AST tree create emit statement which is a sort of intermediate compile step
bool CompileUnit::ProcessASTStatement(IPublicIdentifiers *iPublicIdentifiers, ast::Statement::Ref statement) {
    publicHandler = iPublicIdentifiers;

    auto stmtEmitter = EmitStatementBase::Create(statement);
    if (stmtEmitter == nullptr) {
        fmt::println(stderr, "Compiler, failed to generate emitter for statement {}", ast::NodeTypeToString(statement->Kind()));
        statement->Dump();
        return false;
    }
    stmtEmitter->Process(*this);
    emitStatements.push_back(stmtEmitter);
    return true;
}

// Process all statements and emit the data, this will actually produce the byte code
bool CompileUnit::EmitData(IPublicIdentifiers *iPublicIdentifiers) {
    CreateEmptySegment(".text");

    for(auto stmt : GetEmitStatements()) {
        EnsureChunk();
        auto ofsBefore = GetCurrentWriteAddress();
        if (!stmt->Finalize(*this)) {
            return false;
        }
        fmt::println("  Stmt {} kind={}, before={}, after={}", stmt->EmitId(), (int)stmt->Kind(), ofsBefore, GetCurrentWriteAddress());
    }

    // Link exports to their identifiers
    for(auto &expSymbol : exports) {
        if (!HasIdentifier(expSymbol)) {
            fmt::println(stderr, "Compiler, '{}' is exported but not defined/declared (ignored)", expSymbol);
            continue;
        }
        auto identifier = GetIdentifier(expSymbol);
        auto expIdent = iPublicIdentifiers->GetExport(expSymbol);

        // Link these up
        expIdent->origIdentifier = identifier;
    }

    return true;
}

//
bool CompileUnit::EnsureChunk() {
    if (activeSegment == nullptr) {
        fmt::println(stderr, "Compiler, need at least an active segment");
        return false;
    }
    if (activeSegment->currentChunk != nullptr) {
        return true;
    }
    activeSegment->CreateChunk(GetCurrentWriteAddress());
    return true;
}
bool CompileUnit::GetOrAddSegment(const std::string &name, uint64_t address) {
    if (!segments.contains(name)) {
        auto segment = std::make_shared<Segment>(name, address);
        segments[name] = segment;
    }
    activeSegment = segments.at(name);
    return true;
}

bool CompileUnit::CreateEmptySegment(const std::string &name) {
    if (HaveSegment(name)) {
        activeSegment = segments[name];
        return true;
    }

    auto segment = std::make_shared<Segment>(name);
    segments[name] = segment;
    activeSegment = segment;
    return true;
}

bool CompileUnit::SetActiveSegment(const std::string &name) {
    if (!segments.contains(name)) {
        return false;
    }
    activeSegment = segments.at(name);
    if (activeSegment->currentChunk == nullptr) {
        if (activeSegment->chunks.empty()) {
            fmt::println(stderr, "Linker, Compiled unit can't link - no data!");
            return false;
        }
        activeSegment->currentChunk =  activeSegment->chunks[0];
    }
    return true;
}

bool CompileUnit::HaveSegment(const std::string &name) {
    return segments.contains(name);
}

const Segment::Ref CompileUnit::GetSegment(const std::string segName) {
    if (!segments.contains(segName)) {
        return nullptr;
    }
    return segments.at(segName);
}

size_t CompileUnit::GetSegments(std::vector<Segment::Ref> &outSegments) {
    for(auto [k, v] : segments) {
        outSegments.push_back(v);
    }
    return outSegments.size();
}


Segment::Ref CompileUnit::GetActiveSegment() {
    return activeSegment;
}


size_t CompileUnit::GetSegmentEndAddress(const std::string &name) {
    if (!segments.contains(name)) {
        return 0;
    }
    return segments.at(name)->EndAddress();
}

size_t CompileUnit::Write(const std::vector<uint8_t> &data) {
    if (GetActiveSegment() == nullptr) {
        return 0;
    }
    if (!EnsureChunk()) {
        return 0;
    }
    auto segment = GetActiveSegment();
    auto nWritten = segment->CurrentChunk()->Write(data);

    return nWritten;
}

uint64_t CompileUnit::GetCurrentWriteAddress() {
    if (activeSegment == nullptr) {
        fmt::println(stderr, "Compiler, not active segment!!");
        return 0;
    }
    if (activeSegment->currentChunk == nullptr) {
        // if no chunk - we ship back zero - as we will create one on first write...
        return 0;
    }
    return activeSegment->currentChunk->GetCurrentWriteAddress();
}


//
// Constants
//
bool CompileUnit::HasConstant(const std::string &name) {
    return constants.contains(name);
}

void CompileUnit::AddConstant(const std::string &name, ast::ConstLiteral::Ref constant) {
    constants[name] = std::move(constant);
}

ast::ConstLiteral::Ref CompileUnit::GetConstant(const std::string &name) {
    return constants[name];
}

//
// Private identifiers
//
bool CompileUnit::HasIdentifier(const std::string &ident) {
    if (identifierAddresses.contains(ident)) {
        return true;
    }
    return false;
}

void CompileUnit::AddIdentifier(const std::string &ident, const Identifier &idAddress) {
    identifierAddresses[ident] = std::make_shared<Identifier>(idAddress);
    // Since we send them in as const...
    identifierAddresses[ident]->name = ident;
}


Identifier::Ref CompileUnit::GetIdentifier(const std::string &ident) {
    if (identifierAddresses.contains(ident)) {
        return identifierAddresses[ident];
    }
    return nullptr;
}
