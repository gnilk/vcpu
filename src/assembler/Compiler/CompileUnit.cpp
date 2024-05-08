//
// Created by gnilk on 20.12.23.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include <algorithm>

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
    if (!stmtEmitter->Process(*this)) {
        return false;
    }
    emitStatements.push_back(stmtEmitter);
    return true;
}

// Process all statements and emit the data, this will actually produce the byte code
bool CompileUnit::EmitData(IPublicIdentifiers *iPublicIdentifiers) {
    CreateEmptySegment(Segment::kSegmentType::Code);

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

        // Link these up, this is used in the linker to find the right identifier descriptor...
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

Segment::Ref CompileUnit::GetOrAddSegment(Segment::kSegmentType type, uint64_t address) {
    auto seg = GetSegment(type);
    if (seg == nullptr) {
        seg = std::make_shared<Segment>(type, address);
        segments.push_back(seg);
    }
    activeSegment = seg;
    return activeSegment;
}

Segment::Ref CompileUnit::CreateEmptySegment(Segment::kSegmentType type) {
    return GetOrAddSegment(type, 0);
}
//    if (!segments.contains(type)) {
//        return false;
//    }
//    activeSegment = segments.at(type);
//    if (activeSegment->currentChunk == nullptr) {
//        if (activeSegment->chunks.empty()) {
//            fmt::println(stderr, "Linker, Compiled unit can't link - no data!");
//            return false;
//        }
//        activeSegment->currentChunk =  activeSegment->chunks[0];
//    }
//    return true;
//}

const std::vector<Segment::Ref> &CompileUnit::GetSegments() const {
    return segments;
}

Segment::Ref CompileUnit::GetActiveSegment() {
    return activeSegment;
}


size_t CompileUnit::GetSegmentEndAddress(Segment::kSegmentType type) {
    auto seg = GetSegment(type);
    if (seg == nullptr) {
        return 0;
    }
    return seg->EndAddress();
}

bool CompileUnit::HaveSegment(Segment::kSegmentType type) {
    auto it = std::ranges::find_if(segments.begin(), segments.end(), [type](Segment::Ref v)-> bool {
        return (v->Type() == type);
    });
    return (it != segments.end());
}

const Segment::Ref CompileUnit::GetSegment(Segment::kSegmentType type) {
    auto it = std::ranges::find_if(segments.begin(), segments.end(), [type](Segment::Ref v)-> bool {
        return (v->Type() == type);
    });
    if (it == segments.end()) {
        return nullptr;
    }
    return *it;
}

size_t CompileUnit::GetSegments(std::vector<Segment::Ref> &outSegments) const {
    outSegments.insert(outSegments.end(), segments.begin(), segments.end());
    return outSegments.size();
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
uint64_t CompileUnit::GetCurrentRelativeWriteAddress() {
    if (activeSegment == nullptr) {
        fmt::println(stderr, "Compiler, not active segment!!");
        return 0;
    }
    if (activeSegment->currentChunk == nullptr) {
        // if no chunk - we ship back zero - as we will create one on first write...
        return 0;
    }
    return activeSegment->currentChunk->GetRelativeWriteAddress();

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
