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

        struct IdentifierAddressPlaceholder {
            using Ref = std::shared_ptr<IdentifierAddressPlaceholder>;

            std::string ident;
            Segment::Ref segment = nullptr;
            Segment::DataChunk::Ref chunk = nullptr;
            uint64_t address;
            // For relative jump
            bool isRelative;
            vcpu::OperandSize opSize;
            uint64_t ofsRelative;   // Offset to compute from..
        };

        struct IdentifierResolvePoint {
            vcpu::OperandSize opSize;
            bool isRelative;
            size_t placeholderAddress;
        };

        struct Identifier {
            Segment::Ref segment = nullptr;;
            Segment::DataChunk::Ref chunk = nullptr;
            uint64_t absoluteAddress = 0;
            // New emittor stuff
            std::vector<IdentifierResolvePoint> resolvePoints;
        };

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


        class IPublicIdentifiers {
        public:
            virtual bool HasStructDefinintion(const std::string &typeName) = 0;
            virtual void AddStructDefinition(const StructDefinition &structDefinition) = 0;
            virtual const StructDefinition &GetStructDefinitionFromTypeName(const std::string &typeName) = 0;
            virtual const std::vector<StructDefinition> &StructDefinitions() = 0;

            virtual bool HasExport(const std::string &ident) = 0;
            virtual void AddExport(const std::string &ident) = 0;
            virtual Identifier &GetExport(const std::string &ident) = 0;
            virtual const std::unordered_map<std::string, Identifier> &GetExports() = 0;
        };

    }
}

#endif //IDENTIFIERRELOCATATION_H
