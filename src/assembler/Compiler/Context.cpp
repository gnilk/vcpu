//
// Created by gnilk on 07.03.24.
//

#include "Context.h"

using namespace gnilk;
using namespace gnilk::assembler;

// All these methods should lock the context..


void Context::AddStructDefinition(const StructDefinition &structDefinition) {
    structDefinitions.push_back(structDefinition);
}
bool Context::HasConstant(const std::string &name) {
    return constants.contains(name);
}
void Context::AddConstant(const std::string &name, ast::ConstLiteral::Ref constant) {
    constants[name] = std::move(constant);
}
ast::ConstLiteral::Ref Context::GetConstant(const std::string &name) {
    return constants[name];
}


bool Context::HasStructDefinintion(const std::string &typeName) {
    for(auto &structDef : structDefinitions) {
        if (structDef.ident == typeName) {
            return true;
        }
    }
    return false;
}
const StructDefinition &Context::GetStructDefinitionFromTypeName(const std::string &typeName) {
    for(auto &structDef : structDefinitions) {
        if (structDef.ident == typeName) {
            return structDef;
        }
    }
    // Should never be reached - throw something???
    fmt::println(stderr, "Compiler, critical error; no struct with type name={}", typeName);
    exit(1);
}
bool Context::HasIdentifierAddress(const std::string &ident) {
    return identifierAddresses.contains(ident);
}
void Context::AddIdentifierAddress(const std::string &ident, const IdentifierAddress &idAddress) {
    identifierAddresses[ident] = idAddress;
}
IdentifierAddress &Context::GetIdentifierAddress(const std::string &ident) {
    return identifierAddresses[ident];
}

void Context::AddAddressPlaceholder(const IdentifierAddressPlaceholder::Ref &addressPlaceholder) {
    addressPlaceholders.push_back(addressPlaceholder);
}

uint64_t Context::GetCurrentWriteAddress() {
    return unit.GetCurrentWriteAddress();
}

void Context::Merge() {
    // Make sure this is empty...
    outputdata.clear();
    unit.MergeAllSegments(outputdata);
}

size_t Context::Write(const std::vector<uint8_t> &data) {
    auto nWritten = unit.Write(data);
    return nWritten;
}
