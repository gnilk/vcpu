//
// Created by gnilk on 23.11.23.
//
#include "ast.h"
#include "fmt/format.h"
#include <unordered_map>

using namespace gnilk::ast;

int StatementWriter::lvlIndent = 0;
std::string StatementWriter::strIndent = {};

static std::unordered_map<NodeType, std::string> typeToStringMap = {
    {NodeType::kUnknown,"Unknown"},
    {NodeType::kProgram,"Program"},
    {NodeType::kVarDeclaration,"VarDeclaration"},
    {NodeType::kFunctionDeclaration, "FunctionDeclaration"},
    {NodeType::kIfStatement, "IfStatement"},
    {NodeType::kWhileStatement, "WhileStatement"},
    {NodeType::kCallStatement, "CallStatement"},
    {NodeType::kCallExpression, "CallExpression"},
    {NodeType::kAssignmentStatement,"AssignmentExpression"},
    {NodeType::kProperty,"Property"},
    {NodeType::kObjectLiteral,"ObjectLiteral"},
    {NodeType::kNumericLiteral,"NumericLiteral"},
    {NodeType::kStringLiteral,"StringLiteral"},
    {NodeType::kNullLiteral,"NullLiteral"},
    {NodeType::kIdentifier,"Identifier"},
    {NodeType::kCompareExpression, "CompareExpression"},
    {NodeType::kUnaryExpression, "UnaryExpression"},
    {NodeType::kLogicalExpression, "LogicalExpression"},
    {NodeType::kBinaryExpression,"BinaryExpression"},
    {NodeType::kMemberExpression,"MemberExpression"},
    {NodeType::kCommentStatement,"CommentStatement"},
    {NodeType::kRelativeRegisterLiteral, "RelativeRegisterLiteral"},
    {NodeType::kArrayLiteral, "ArrayLiteral"},
    {NodeType::kStructLiteral, "StructLiteral"},
    {NodeType::kExportStatement, "ExportStatement"},
};
static const std::string unknownNodeType="<unknown>";

namespace gnilk::ast {
    const std::string &NodeTypeToString(NodeType type) {
        if (typeToStringMap.contains(type)) {
            return typeToStringMap[type];
        }
        fmt::println(stderr, "Missing node type in 'typeToStringMap' for type nb. {}", (int)type);
        return unknownNodeType;
    }
}
