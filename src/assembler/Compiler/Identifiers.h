//
// Created by gnilk on 11.01.2024.
//

#ifndef IDENTIFIERRELOCATATION_H
#define IDENTIFIERRELOCATATION_H

#include <string>
#include <stdint.h>
#include <memory>

#include "InstructionSet.h"
#include "Linker/Segment.h"

namespace gnilk {
    namespace assembler {

        // Consider refactoring this...
        // We have identifiers (or symbols) of various kind (local, exported, etc..) -> one structure should be enough
        // Exports is an attribute to a symbol...
        // Resolve points is the place where symbol addresses should be placed when linking

        struct IdentifierResolvePoint {
            Segment::Ref segment = nullptr;
            Segment::DataChunk::Ref chunk = nullptr;

            vcpu::OperandSize opSize;
            bool isRelative;

            // This is relative the chunk load address
            size_t placeholderAddress;
        };

        // This is a public identifier in a compile unit - it is only reachable within the compile unit.
        // IF an identifier should be accessable across compile units it must be marked explicitly as 'export'.
        struct Identifier {
            using Ref = std::shared_ptr<Identifier>;

            std::string name;
            Segment::Ref segment = nullptr;;
            Segment::DataChunk::Ref chunk = nullptr;
            uint64_t absoluteAddress = 0;
            uint64_t relativeAddress = 0;
            std::vector<IdentifierResolvePoint> resolvePoints;
        };

        // inherit from identifier - this is used to gain access to the resolve points
        // an exported identifier is resolved last - and has it's own set of resolve points which are
        // tracked across compile-units within the same context...
        struct ExportIdentifier : public Identifier {
            using Ref = std::shared_ptr<ExportIdentifier>;

            Identifier::Ref origIdentifier = nullptr;    // when identifier is exported - this points to the declared identifier
        };

        // I should probably clean these up a bit...
        struct ImportIdentifier : public Identifier {
            using Ref = std::shared_ptr<ImportIdentifier>;
        };

        struct StructMember {
            std::string ident;
            size_t offset;
            size_t byteSize;
            ast::Statement::Ref declarationStatement;
        };

        struct StructDefinition {
            using Ref = std::shared_ptr<StructDefinition>;

            size_t NumMembers() const {
                return members.size();
            }
            // This is more or less fine - assuming we never go outside the vector
            // However, if rewriting everything to use pointers maybe we should for these two as well
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

        // Interface to access public members - these are generally exported symbols implemented by Context and CompileUnit
        // Mainly since I have a circular dependency between them...
        class IPublicIdentifiers {
        public:
            virtual bool HasStructDefinintion(const std::string &typeName) = 0;
            // keep this as reference - we will make a copy - but perhaps doesn't matter if it is trivial copyable...
            virtual void AddStructDefinition(const StructDefinition &structDefinition) = 0;
            virtual const StructDefinition::Ref GetStructDefinitionFromTypeName(const std::string &typeName) = 0;
            virtual const std::vector<StructDefinition::Ref> &StructDefinitions() = 0;

            virtual bool HasExport(const std::string &ident) = 0;
            virtual void AddExport(const std::string &ident) = 0;
            virtual ExportIdentifier::Ref GetExport(const std::string &ident) = 0;
            virtual const std::unordered_map<std::string, ExportIdentifier::Ref> &GetExports() = 0;
        };

    }
}

#endif //IDENTIFIERRELOCATATION_H
