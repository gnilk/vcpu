//
// Created by gnilk on 20.12.23.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include "CompiledUnit.h"
#include "Compiler/Context.h"

using namespace gnilk;
using namespace gnilk::assembler;

void CompiledUnit::Clear() {
    emitStatements.clear();
}

// Process the AST tree create emit statement which is a sort of intermediate compile step
bool CompiledUnit::ProcessASTStatement(Context &context, ast::Statement::Ref statement) {
    auto stmtEmitter = EmitStatementBase::Create(statement);
    if (stmtEmitter == nullptr) {
        fmt::println(stderr, "Compiler, failed to generate emitter for statement {}", ast::NodeTypeToString(statement->Kind()));
        statement->Dump();
        return false;
    }
    stmtEmitter->Process(context);
    emitStatements.push_back(stmtEmitter);
    return true;
}

// Process all statements and emit the data, this will actually produce the byte code
bool CompiledUnit::EmitData(Context &context) {
    for(auto stmt : GetEmitStatements()) {
        auto ofsBefore = context.Data().size();
        if (!stmt->Finalize(context)) {
            return false;
        }
        fmt::println("  Stmt {} kind={}, before={}, after={}", stmt->EmitId(), (int)stmt->Kind(), ofsBefore, context.Data().size());
    }
    return true;
}

//
bool CompiledUnit::EnsureChunk() {
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
bool CompiledUnit::GetOrAddSegment(const std::string &name, uint64_t address) {
    if (!segments.contains(name)) {
        auto segment = std::make_shared<Segment>(name, address);
        segments[name] = segment;
    }
    activeSegment = segments.at(name);
    return true;
}

bool CompiledUnit::CreateEmptySegment(const std::string &name) {
    if (HaveSegment(name)) {
        activeSegment = segments[name];
        return true;
    }

    auto segment = std::make_shared<Segment>(name);
    segments[name] = segment;
    activeSegment = segment;
    return true;
}

bool CompiledUnit::SetActiveSegment(const std::string &name) {
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
bool CompiledUnit::HaveSegment(const std::string &name) {
    return segments.contains(name);
}

const Segment::Ref CompiledUnit::GetSegment(const std::string segName) {
    if (!segments.contains(segName)) {
        return nullptr;
    }
    return segments.at(segName);
}

size_t CompiledUnit::GetSegments(std::vector<Segment::Ref> &outSegments) {
    for(auto [k, v] : segments) {
        outSegments.push_back(v);
    }
    return outSegments.size();
}


Segment::Ref CompiledUnit::GetActiveSegment() {
    return activeSegment;
}


size_t CompiledUnit::GetSegmentEndAddress(const std::string &name) {
    if (!segments.contains(name)) {
        return 0;
    }
    return segments.at(name)->EndAddress();
}



size_t CompiledUnit::Write(Context &context, const std::vector<uint8_t> &data) {
    if (context.GetActiveSegment() == nullptr) {
        return 0;
    }
    auto segment = context.GetActiveSegment();
    if (segment->CurrentChunk() == nullptr) {
        return 0;
    }
    auto nWritten = segment->CurrentChunk()->Write(data);

    return nWritten;
}

uint64_t CompiledUnit::GetCurrentWriteAddress() {
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

// FIXME: REmove this!
const std::vector<uint8_t> &CompiledUnit::Data(Context &context) {
    if (!context.GetOrAddSegment("link_out", 0x00)) {
        static std::vector<uint8_t> empty = {};
        return empty;
    }
    return context.GetActiveSegment()->CurrentChunk()->Data();
}
