//
// Created by gnilk on 15.12.23.
//

#ifndef COMPILER_H
#define COMPILER_H

#include <functional>

#include "Context.h"
#include "InstructionSet.h"
#include "ast/ast.h"
#include "Linker/CompiledUnit.h"
#include "IdentifierRelocatation.h"
#include "Linker/BaseLinker.h"
#include "EmitStatement.h"
#include "StmtEmitter.h"

namespace gnilk {
    namespace assembler {

        class Compiler {
        public:
            Compiler() = default;
            virtual ~Compiler() = default;


            bool CompileAndLink(gnilk::ast::Program::Ref program);
            bool Compile(gnilk::ast::Program::Ref program);
            bool Link();

            void SetLinker(BaseLinker *newLinker) {
                linker = newLinker;
            }

            CompiledUnit &GetCompiledUnit() {
                //return unit;
                return context.Unit();
            }


            // TEMP TEMP
            const std::vector<uint8_t> &Data() {
                //return linker->Data();
                return context.Data();
            }
        protected:


            using DeferredOpSizeHandler = std::function<void(uint8_t opSize)>;
            DeferredOpSizeHandler cbDeferredOpSize = nullptr;
            void DeferEmitOpSize(DeferredOpSizeHandler emitOpSizeCallback);
            void ResetDeferEmitOpSize();
            bool IsOpSizeDeferred();
            void EmitOpSize(uint8_t opSize);
        private:
            Context context;

            BaseLinker *linker = nullptr;
            std::vector<EmitStatementBase::Ref> emitStatements;

            std::vector<uint8_t> linkdata;
        };
    }
}



#endif //COMPILER_H
