//
// Created by gnilk on 07.03.24.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include <vector>

#include "CompiledUnit.h"
#include "Linker/Segment.h"
#include "IdentifierRelocatation.h"
#include "ast/ast.h"


namespace gnilk {
    namespace assembler {

        struct StructMember {
            std::string ident;
            size_t offset;
            size_t byteSize;
            ast::Statement::Ref declarationStatement;
        };

        struct StructDefinition {
            size_t NumMembers() const {
                return members.size();
            }
            const StructMember &GetMemberAt(size_t idx) const {
                return members[idx];
            }
            const StructMember &GetMember(const std::string &name) const {
                auto member = std::find_if(members.begin(), members.end(), [&name](const StructMember &member) {
                    return (member.ident == name);
                });
                return *member;
            }
            bool HasMember(const std::string &name) const {
                auto member = std::find_if(members.begin(), members.end(), [&name](const StructMember &member) {
                    return (member.ident == name);
                });
                return (member != members.end());
            }

            std::string ident;
            size_t byteSize;
            std::vector<StructMember> members;
        };

        class Context {
        public:
            Context() = default;
            virtual ~Context() = default;

            void Clear();

            const std::vector<StructDefinition> &StructDefinitions();

            bool HasStructDefinintion(const std::string &typeName);
            void AddStructDefinition(const StructDefinition &structDefinition);
            const StructDefinition &GetStructDefinitionFromTypeName(const std::string &typeName);

            bool HasConstant(const std::string &name);
            void AddConstant(const std::string &name, ast::ConstLiteral::Ref constant);
            ast::ConstLiteral::Ref GetConstant(const std::string &name);

            bool HasExport(const std::string &ident);
            void AddExport(const std::string &ident);
            Identifier &GetExport(const std::string &ident);
            const std::unordered_map<std::string, Identifier> &GetExports() {
                return publicIdentifiers;
            }



            bool HasIdentifier(const std::string &ident);
            void AddIdentifier(const std::string &ident, const Identifier &idAddress);
            Identifier &GetIdentifier(const std::string &ident);
            const std::unordered_map<std::string, Identifier> &GetIdentifiers() {
                return identifierAddresses;
            }


            void AddAddressPlaceholder(const IdentifierAddressPlaceholder::Ref &addressPlaceholder);

            CompiledUnit &CreateUnit();
            const std::vector<CompiledUnit> &GetUnits() {
                return units;
            }

            CompiledUnit &CurrentUnit() {
                return units.back();
            }

            void Merge();

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

            // TEMP
            void MergeAllSegments(std::vector<uint8_t> &out);

            uint64_t GetCurrentWriteAddress();
            size_t Write(const std::vector<uint8_t> &data);
            std::vector<uint8_t> &Data() {
                return outputdata;
            }

        protected:
            // TEMP:
            std::vector<uint8_t> outputdata;

            // Should be list...
            //CompiledUnit unit;
            std::vector<CompiledUnit> units;

            // Segments and chunks are here as they span multiple units..
            std::unordered_map<std::string, Segment::Ref> segments;
            Segment::Ref activeSegment = nullptr;

            std::unordered_map<std::string, Identifier> publicIdentifiers;


            // This is a type-definition - should they always be exported?
            std::vector<StructDefinition> structDefinitions;

            // FIXME: move to compiled unit
            std::vector<IdentifierAddressPlaceholder::Ref> addressPlaceholders;
            std::unordered_map<std::string, Identifier> identifierAddresses;

            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;

        };

    };
}



#endif //CONTEXT_H
