// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptAst.h"

namespace Urho3D
{

enum class RbScriptTypeKind
{
    Invalid,
    Void,
    Bool,
    Int,
    UInt,
    Float,
    Double,
    String,
    Vector2,
    Vector3,
    Quaternion,
    Color,
    Node,
    Component,
    Resource,
    Variant,
    Optional,
    Array,
    Map,
    User,
};

struct URHO3D_API RbScriptType
{
    RbScriptTypeKind kind{RbScriptTypeKind::Invalid};
    ea::string name;
    ea::string keyType;
    ea::string elementType;

    bool IsValid() const { return kind != RbScriptTypeKind::Invalid; }
    bool IsNumeric() const;
    bool IsAssignableFrom(const RbScriptType& value) const;
    ea::string ToString() const;

    bool operator==(const RbScriptType& rhs) const;
    bool operator!=(const RbScriptType& rhs) const { return !(*this == rhs); }
};

struct URHO3D_API RbScriptFunctionSignature
{
    ea::string name;
    RbScriptType returnType;
    ea::vector<RbScriptType> parameterTypes;
    bool asynchronous{false};
};

class URHO3D_API RbScriptTypeRegistry
{
public:
    RbScriptTypeRegistry();

    void RegisterType(const RbScriptType& type);
    void RegisterAlias(const ea::string& alias, const ea::string& target);
    void RegisterFunction(const RbScriptFunctionSignature& signature);

    RbScriptType Resolve(const ea::string& name) const;
    const RbScriptFunctionSignature* FindFunction(const ea::string& name) const;
    bool HasType(const ea::string& name) const;

private:
    ea::unordered_map<ea::string, RbScriptType> types_;
    ea::unordered_map<ea::string, ea::string> aliases_;
    ea::unordered_map<ea::string, RbScriptFunctionSignature> functions_;
};

class URHO3D_API RbScriptTypeChecker
{
public:
    explicit RbScriptTypeChecker(const RbScriptTypeRegistry& registry);

    bool Check(const RbScriptModule& module);
    const ea::vector<RbScriptDiagnostic>& GetDiagnostics() const { return diagnostics_; }

private:
    void AddDiagnostic(RbScriptDiagnosticSeverity severity, const ea::string& code,
        const ea::string& message, const RbScriptSourceSpan& span);
    void CheckScript(const RbScriptScript& script);
    void CheckFunction(const RbScriptFunction& function);
    void CheckStatement(const RbScriptStatement& statement, const RbScriptType& expectedReturn);
    RbScriptType InferExpression(const RbScriptExpression& expression);
    RbScriptType ResolveOrReport(const ea::string& name, const RbScriptSourceSpan& span);

    const RbScriptTypeRegistry& registry_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    ea::unordered_map<ea::string, RbScriptType> symbols_;
    ea::unordered_map<ea::string, RbScriptFunctionSignature> functions_;
};

} // namespace Urho3D
