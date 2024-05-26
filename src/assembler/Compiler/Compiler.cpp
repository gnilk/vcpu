//
// Created by gnilk on 15.12.23.
//

#include "Compiler.h"
#include "fmt/format.h"

#include <memory>
#include <unordered_map>

#include "InstructionSetV1/InstructionSetDefV1.h"
#include "CompileUnit.h"

#include "elfio/elfio.hpp"
#include "Linker/RawLinker.h"
#include "Linker/ElfLinker.h"

#include "DurationTimer.h"

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

    DurationTimer timer;

    // Create the unit
    auto &unit = context.CreateUnit();

    // Process all statements within our unit...
    for(auto &statement : program->Body()) {
        if (!unit.ProcessASTStatement(&context, statement)) {
            return false;
        }
    }

    // Finalize and generate data to context.
    if (!unit.EmitData(&context)) {
        return false;
    }

    msCompileDuration = timer.Sample();

    return true;
}


//
// Compiler front-end also allows linkage - stupid?
//
bool Compiler::Link() {
    if (linker == nullptr) {
        linker = std::make_shared<RawLinker>();
    }
    DurationTimer timer;
    auto result = linker->Link(context);
    msLinkDuration = timer.Sample();
    return result;
}









