//
// Created by gnilk on 15.12.23.
//

#ifndef COMPILER_H
#define COMPILER_H

#include <functional>

#include "Context.h"
#include "InstructionSet.h"
#include "ast/ast.h"
#include "CompileUnit.h"
#include "Identifiers.h"
#include "Linker/BaseLinker.h"
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

            Context &GetContext() {
                //return unit;
                return context;
            }

            const std::vector<uint8_t> &Data() {
                return linker->Data();
                //return context.Data();
            }

            double GetCompileDurationMS() {
                return msCompileDuration;
            }
            double GetLinkDurationMS() {
                return msLinkDuration;
            }

        private:
            double msCompileDuration = 0.0;
            double msLinkDuration = 0.0;
            Context context;
            BaseLinker *linker = nullptr;

            std::vector<uint8_t> linkdata;
        };
    }
}



#endif //COMPILER_H
