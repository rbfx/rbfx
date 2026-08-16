// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptDefs.h"

#include <memory>
#include <vector>

namespace Urho3D
{

enum class RbScriptExpressionKind
{
    Invalid,
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    BooleanLiteral,
    NullLiteral,
    Unary,
    Binary,
    Call,
    Member,
};

struct URHO3D_API RbScriptExpression
{
    RbScriptExpressionKind kind{RbScriptExpressionKind::Invalid};
    RbScriptSourceSpan span;
    ea::string text;
    std::vector<std::unique_ptr<RbScriptExpression>> children;
};

enum class RbScriptStatementKind
{
    Empty,
    Block,
    VariableDeclaration,
    Expression,
    Return,
    If,
    While,
    Break,
    Continue,
    Emit,
};

struct URHO3D_API RbScriptStatement
{
    RbScriptStatementKind kind{RbScriptStatementKind::Empty};
    RbScriptSourceSpan span;
    ea::string name;
    ea::string typeName;
    std::unique_ptr<RbScriptExpression> expression;
    std::vector<std::unique_ptr<RbScriptStatement>> body;
    std::vector<std::unique_ptr<RbScriptStatement>> elseBody;
};

struct URHO3D_API RbScriptParameter
{
    ea::string name;
    ea::string typeName;
    std::unique_ptr<RbScriptExpression> defaultValue;
    RbScriptSourceSpan span;
};

struct URHO3D_API RbScriptField
{
    ea::string name;
    ea::string typeName;
    ea::vector<ea::string> attributes;
    std::unique_ptr<RbScriptExpression> initializer;
    RbScriptSourceSpan span;
};

struct URHO3D_API RbScriptFunction
{
    ea::string name;
    ea::string returnType{"void"};
    ea::vector<ea::string> attributes;
    std::vector<RbScriptParameter> parameters;
    std::vector<std::unique_ptr<RbScriptStatement>> body;
    bool asynchronous{false};
    bool eventHandler{false};
    RbScriptSourceSpan span;
};

struct URHO3D_API RbScriptScript
{
    ea::string name;
    ea::string baseType;
    ea::vector<ea::string> attributes;
    ea::vector<RbScriptField> fields;
    std::vector<RbScriptFunction> functions;
    RbScriptSourceSpan span;
};

struct URHO3D_API RbScriptModule
{
    ea::string name;
    ea::vector<ea::string> imports;
    std::vector<RbScriptScript> scripts;
    ea::vector<RbScriptDiagnostic> diagnostics;

    bool IsValid() const;
};

} // namespace Urho3D
