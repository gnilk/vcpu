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

            bool HasIdentifierAddress(const std::string &ident);
            void AddIdentifierAddress(const std::string &ident, const IdentifierAddress &idAddress);
            const IdentifierAddress &GetIdentifierAddress(const std::string &ident);

            void AddAddressPlaceholder(const IdentifierAddressPlaceholder &addressPlaceholder);

            CompiledUnit &Unit() {
                return unit;
            }

        protected:
            CompiledUnit unit;

            std::vector<StructDefinition> structDefinitions;
            std::vector<IdentifierAddressPlaceholder> addressPlaceholders;
            std::unordered_map<std::string, IdentifierAddress> identifierAddresses;
            std::unordered_map<std::string, ast::ConstLiteral::Ref> constants;

        };

    };
}



#endif //CONTEXT_H
