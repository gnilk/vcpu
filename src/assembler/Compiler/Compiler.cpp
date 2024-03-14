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

#include "StmtEmitter.h"

using namespace gnilk;
using namespace gnilk::assembler;

bool Compiler::CompileAndLink(ast::Program::Ref program) {
    if (!Compile(program)) {
        return false;
    }
    //return Link();
    return true;
}

static void VectorReplaceAt(std::vector<uint8_t> &data, uint64_t offset, int64_t newValue, vcpu::OperandSize opSize);

bool Compiler::Compile(gnilk::ast::Program::Ref program) {

    // Some unit-tests reuse the Compiler instance - this probably not a good idea...
    // Perhaps a specific 'reset' call...
    context = Context();
    emitStatements.clear();

//    context.Unit().Clear();

    // TEMP!
    context.Unit().GetOrAddSegment(".text", 0);

    // This can now be multi-threaded - fancy, eh?
    size_t idxStmtCounter = 0;
    for(auto &statement : program->Body()) {
        auto stmtEmitter = EmitStatementBase::Create(statement);
        if (stmtEmitter == nullptr) {
            fmt::println(stderr, "Compiler, failed to generate emitter for statement {}", ast::NodeTypeToString(statement->Kind()));
            statement->Dump();
            return false;
        }
        stmtEmitter->Process(context);
        emitStatements.push_back(stmtEmitter);
    }

    // This will produce a flat binary...
    for(auto stmt : emitStatements) {
        auto ofsBefore = context.Data().size();
        stmt->Finalize(context);
        fmt::println("  Stmt {}, before={}, after={}", stmt->emitid, ofsBefore, context.Data().size());
    }

    // Resolve - this needs to move to the linker...
    auto &data = context.Data();
    for(auto &[symbol, identifier] : context.identifierAddresses) {
        fmt::println("  {} @ {}", symbol, identifier.address);
        for(auto &resolvePoint : identifier.resolvePoints) {
            if (!resolvePoint.isRelative) {
                VectorReplaceAt(data, resolvePoint.placeholderAddress, identifier.address, resolvePoint.opSize);
            } else {
                int64_t offset = static_cast<int64_t>(identifier.address) - static_cast<int64_t>(resolvePoint.placeholderAddress);
                if (offset < 0) {
                    offset -= 1;
                }
                VectorReplaceAt(data, resolvePoint.placeholderAddress, offset, resolvePoint.opSize);
            }
        }
    }

    return true;
}

static void VectorReplaceAt(std::vector<uint8_t> &data, uint64_t offset, int64_t newValue, vcpu::OperandSize opSize) {
    if (data.size() < vcpu::ByteSizeOfOperandSize(opSize)) {
        return;
    }
    if (offset > (data.size() - vcpu::ByteSizeOfOperandSize(opSize))) {
        return;
    }
    switch(opSize) {
        case vcpu::OperandSize::Byte :
            data[offset++] = (newValue & 0xff);
            break;
        case vcpu::OperandSize::Word :
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            break;
        case vcpu::OperandSize::DWord :
            data[offset++] = (newValue>>24 & 0xff);
            data[offset++] = (newValue>>16 & 0xff);
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            break;
        case vcpu::OperandSize::Long :
            data[offset++] = (newValue>>56 & 0xff);
            data[offset++] = (newValue>>48 & 0xff);
            data[offset++] = (newValue>>40 & 0xff);
            data[offset++] = (newValue>>32 & 0xff);
            data[offset++] = (newValue>>24 & 0xff);
            data[offset++] = (newValue>>16 & 0xff);
            data[offset++] = (newValue>>8 & 0xff);
            data[offset++] = (newValue & 0xff);
            return;
    }
}


//
// This is a simple single-unit self-linking thingie - produces something like a a.out file...
// We should split this to it's own structure
//
bool Compiler::Link() {
    if (linker == nullptr) {
        static DummyLinker dummyLinker;
        linker = &dummyLinker;
    }
    //DummyLinker linker;
    //ElfLinker linker;
    return linker->Link(context.Unit(), context.identifierAddresses, context.addressPlaceholders);
}









