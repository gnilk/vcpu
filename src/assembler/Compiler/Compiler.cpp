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
#include "CompiledUnit.h"

#include "elfio/elfio.hpp"
#include "Linker/DummyLinker.h"
#include "Linker/ElfLinker.h"

#include "StmtEmitter.h"

using namespace gnilk;
using namespace gnilk::assembler;

bool Compiler::CompileAndLink(ast::Program::Ref program) {
    // Make sure we operate on a clean context - if someone is reusing the instance...
    context = Context();
    if (!Compile(program)) {
        return false;
    }
    return Link();
}

bool Compiler::Compile(gnilk::ast::Program::Ref program) {

    //context.GetOrAddSegment(".text", 0);
    context.CreateEmptySegment(".text");

    // Create the unit
    auto &unit = context.CreateUnit();
    // Process all statements within our unit...
    for(auto &statement : program->Body()) {
        if (!unit.ProcessASTStatement(context, statement)) {
            return false;
        }
    }

    // FIXME: this should be in unit...
    for(auto stmt : unit.GetEmitStatements()) {
        auto ofsBefore = context.Data().size();
        stmt->Finalize(context);
        fmt::println("  Stmt {} kind={}, before={}, after={}", stmt->EmitId(), (int)stmt->Kind(), ofsBefore, context.Data().size());
    }


    return true;
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
    return linker->Link(context);
}









