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
    if (!Compile(program)) {
        return false;
    }
    return Link();
}

bool Compiler::Compile(gnilk::ast::Program::Ref program) {

    // Some unit-tests reuse the Compiler instance - this probably not a good idea...
    // Perhaps a specific 'reset' call...

    context = Context();
    context.GetOrAddSegment(".text", 0);

    auto &unit = context.Unit();
    for(auto &statement : program->Body()) {
        if (!unit.ProcessASTStatement(context, statement)) {
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
    if (linker == nullptr) {
        static DummyLinker dummyLinker;
        linker = &dummyLinker;
    }
    return linker->Link(context);
}









