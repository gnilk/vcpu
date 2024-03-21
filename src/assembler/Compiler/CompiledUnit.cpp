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

uint64_t CompiledUnit::GetCurrentWriteAddress(Context &context) {
    if (context.GetActiveSegment() == nullptr) {
        fmt::println(stderr, "Compiler, not active segment!!");
        return 0;
    }
    if (context.GetActiveSegment()->CurrentChunk()) {
        // if no chunk we supply one on first write, this just means no explicit .org statement
        return 0;
    }
    return context.GetActiveSegment()->CurrentChunk()->GetCurrentWriteAddress();
}

const std::vector<uint8_t> &CompiledUnit::Data(Context &context) {
    if (!context.GetOrAddSegment("link_out", 0x00)) {
        static std::vector<uint8_t> empty = {};
        return empty;
    }
    return context.GetActiveSegment()->CurrentChunk()->Data();
}
