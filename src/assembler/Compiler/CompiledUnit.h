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
#include "IdentifierRelocatation.h"


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
            bool EmitData(Context &context);

            size_t Write(Context &context, const std::vector<uint8_t> &data);

            const std::vector<EmitStatementBase::Ref> &GetEmitStatements() {
                return emitStatements;
            }
            // temp?
            const std::vector<uint8_t> &Data(Context &context);

            // Segment handling
            bool EnsureChunk();
            bool CreateEmptySegment(const std::string &name);
            bool GetOrAddSegment(const std::string &name, uint64_t address);
            bool SetActiveSegment(const std::string &name);
            bool HaveSegment(const std::string &name);
            Segment::Ref GetActiveSegment();
            size_t GetSegments(std::vector<Segment::Ref> &outSegments);
            const Segment::Ref GetSegment(const std::string segName);
            size_t GetSegmentEndAddress(const std::string &name);
            uint64_t GetCurrentWriteAddress();

        private:
            std::vector<EmitStatementBase::Ref> emitStatements;

            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            std::unordered_map<std::string, Identifier> identifierAddresses;
            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;
        };
    }
}

#endif //COMPILEDUNIT_H
