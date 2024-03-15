//
// Created by gnilk on 07.03.24.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include <vector>

#include "Linker/CompiledUnit.h"
#include "IdentifierRelocatation.h"
#include "ast/ast.h"


namespace gnilk {
    namespace assembler {
        class Compiler;

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
            friend Compiler;
        public:
            Context() = default;
            virtual ~Context() = default;


            const std::vector<StructDefinition> &StructDefinitions();

            bool HasStructDefinintion(const std::string &typeName);
            void AddStructDefinition(const StructDefinition &structDefinition);
            const StructDefinition &GetStructDefinitionFromTypeName(const std::string &typeName);

            bool HasConstant(const std::string &name);
            void AddConstant(const std::string &name, ast::ConstLiteral::Ref constant);
            ast::ConstLiteral::Ref GetConstant(const std::string &name);

            // FIXME: Once emitters are 'working' drop 'Address' for this...
            bool HasIdentifierAddress(const std::string &ident);
            void AddIdentifierAddress(const std::string &ident, const IdentifierAddress &idAddress);
            IdentifierAddress &GetIdentifierAddress(const std::string &ident);

            void AddAddressPlaceholder(const IdentifierAddressPlaceholder::Ref &addressPlaceholder);

            CompiledUnit &Unit() {
                return unit;
            }

            void Merge();


            uint64_t GetCurrentWriteAddress();
            size_t Write(const std::vector<uint8_t> &data);
            std::vector<uint8_t> &Data() {
                return outputdata;
            }

        protected:
            // TEMP:
            std::vector<uint8_t> outputdata;

            CompiledUnit unit;

            std::vector<StructDefinition> structDefinitions;
            //std::vector<IdentifierAddressPlaceholder> addressPlaceholders;
            std::vector<IdentifierAddressPlaceholder::Ref> addressPlaceholders;
            std::unordered_map<std::string, IdentifierAddress> identifierAddresses;
            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;

        };

    };
}



#endif //CONTEXT_H
