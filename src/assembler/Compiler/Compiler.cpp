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
        EmitStatement::Ref emitStatement = std::make_shared<EmitStatement>(idxStmtCounter, context, statement);

        if (!emitStatement->Process()) {
            fmt::println(stderr, "Error on stmt counter={}", idxStmtCounter);
            statement->Dump();
            return false;
        }

        // Advance to next stmt...
        // FIXME: lock the queue and counter here...

        idxStmtCounter++;
        emitStatements.push_back(emitStatement);
    }

    // Now sort the array
    std::sort(emitStatements.begin(), emitStatements.end(), [](const EmitStatement::Ref &a, const EmitStatement::Ref &b) {
       return (a->id < b->id);
    });
    // Write them out
    WriteEmitStatements();

    return true;
}
void Compiler::WriteEmitStatements() {
    for(auto &stmt : emitStatements) {
        stmt->Finalize();
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









