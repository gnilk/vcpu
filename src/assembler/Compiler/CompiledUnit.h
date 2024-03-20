//
// Created by gnilk on 20.12.23.
//

#ifndef COMPILEDUNIT_H
#define COMPILEDUNIT_H

#include <string>
#include <vector>
#include <stdint.h>

#include "ast/ast.h"
#include "StmtEmitter.h"
#include "InstructionSet.h"
#include "Linker/Segment.h"

namespace gnilk {
    namespace assembler {
        // Forward declare so we can use the reference but avoid circular references
        class Context;

        // This should represent a single compiler unit
        // Segment handling should be moved out from here -> they belong to the context as they are shared between compiled units
        class CompiledUnit {
        public:
            CompiledUnit() = default;
            virtual ~CompiledUnit() = default;

            void Clear();

            bool ProcessASTStatement(Context &context, ast::Statement::Ref statement);

            size_t Write(Context &context, const std::vector<uint8_t> &data);

            uint64_t GetCurrentWriteAddress(Context &context);
            const std::vector<EmitStatementBase::Ref> &GetEmitStatements() {
                return emitStatements;
            }
            // temp?
            const std::vector<uint8_t> &Data(Context &context);
        private:
            std::vector<EmitStatementBase::Ref> emitStatements;
        };
    }
}

#endif //COMPILEDUNIT_H
