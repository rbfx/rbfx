// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptType.h"

namespace Urho3D
{

namespace
{

RbScriptType MakeType(RbScriptTypeKind kind, const char* name)
{
    RbScriptType type;
    type.kind = kind;
    type.name = name;
    return type;
}

bool IsComparison(const ea::string& op)
{
    return op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=";
}

bool IsLogical(const ea::string& op)
{
    return op == "&&" || op == "||";
}

ea::string Trim(const ea::string& value)
{
    unsigned begin = 0;
    unsigned end = value.size();
    while (begin < end && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\n'))
        ++begin;
    while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\n'))
        --end;
    return value.substr(begin, end - begin);
}

} // namespace

bool RbScriptType::IsNumeric() const
{
    return kind == RbScriptTypeKind::Int || kind == RbScriptTypeKind::UInt
        || kind == RbScriptTypeKind::Float || kind == RbScriptTypeKind::Double;
}

bool RbScriptType::IsAssignableFrom(const RbScriptType& value) const
{
    if (!IsValid() || !value.IsValid())
        return false;
    if (*this == value || kind == RbScriptTypeKind::Variant || value.kind == RbScriptTypeKind::Variant)
        return true;
    if (IsNumeric() && value.IsNumeric())
    {
        if (kind == RbScriptTypeKind::Double || kind == RbScriptTypeKind::Float)
            return true;
        return kind == RbScriptTypeKind::Int && value.kind == RbScriptTypeKind::Int;
    }
    if (kind == RbScriptTypeKind::Optional && elementType == value.name)
        return true;
    return false;
}

ea::string RbScriptType::ToString() const
{
    if (!name.empty())
        return name;
    switch (kind)
    {
    case RbScriptTypeKind::Void: return "void";
    case RbScriptTypeKind::Bool: return "bool";
    case RbScriptTypeKind::Int: return "i32";
    case RbScriptTypeKind::UInt: return "u32";
    case RbScriptTypeKind::Float: return "f32";
    case RbScriptTypeKind::Double: return "f64";
    case RbScriptTypeKind::String: return "String";
    case RbScriptTypeKind::Vector2: return "Vector2";
    case RbScriptTypeKind::Vector3: return "Vector3";
    case RbScriptTypeKind::Quaternion: return "Quaternion";
    case RbScriptTypeKind::Color: return "Color";
    case RbScriptTypeKind::Node: return "Node";
    case RbScriptTypeKind::Component: return "Component";
    case RbScriptTypeKind::Resource: return "Resource";
    case RbScriptTypeKind::Variant: return "Variant";
    default: return "invalid";
    }
}

bool RbScriptType::operator==(const RbScriptType& rhs) const
{
    return kind == rhs.kind && name == rhs.name && keyType == rhs.keyType && elementType == rhs.elementType;
}

RbScriptTypeRegistry::RbScriptTypeRegistry()
{
    RegisterType(MakeType(RbScriptTypeKind::Void, "void"));
    RegisterType(MakeType(RbScriptTypeKind::Bool, "bool"));
    RegisterType(MakeType(RbScriptTypeKind::Int, "i32"));
    RegisterType(MakeType(RbScriptTypeKind::UInt, "u32"));
    RegisterType(MakeType(RbScriptTypeKind::Float, "f32"));
    RegisterType(MakeType(RbScriptTypeKind::Double, "f64"));
    RegisterType(MakeType(RbScriptTypeKind::String, "String"));
    RegisterType(MakeType(RbScriptTypeKind::Vector2, "Vector2"));
    RegisterType(MakeType(RbScriptTypeKind::Vector3, "Vector3"));
    RegisterType(MakeType(RbScriptTypeKind::Quaternion, "Quaternion"));
    RegisterType(MakeType(RbScriptTypeKind::Color, "Color"));
    RegisterType(MakeType(RbScriptTypeKind::Node, "Node"));
    RegisterType(MakeType(RbScriptTypeKind::Component, "Component"));
    RegisterType(MakeType(RbScriptTypeKind::Resource, "Resource"));
    RegisterType(MakeType(RbScriptTypeKind::Variant, "Variant"));

    RegisterAlias("Int", "i32");
    RegisterAlias("UInt", "u32");
    RegisterAlias("Float", "f32");
    RegisterAlias("Double", "f64");
    RegisterAlias("NodeBehavior", "Node");
    RegisterAlias("NodeRef", "Node");
}

void RbScriptTypeRegistry::RegisterType(const RbScriptType& type)
{
    if (!type.name.empty())
        types_[type.name] = type;
}

void RbScriptTypeRegistry::RegisterAlias(const ea::string& alias, const ea::string& target)
{
    aliases_[alias] = target;
}

void RbScriptTypeRegistry::RegisterFunction(const RbScriptFunctionSignature& signature)
{
    functions_[signature.name] = signature;
}

RbScriptType RbScriptTypeRegistry::Resolve(const ea::string& name) const
{
    ea::string resolved = name;
    for (unsigned i = 0; i < 8; ++i)
    {
        const auto alias = aliases_.find(resolved);
        if (alias == aliases_.end())
            break;
        resolved = alias->second;
    }

    const auto found = types_.find(resolved);
    if (found != types_.end())
        return found->second;

    const ea::string arrayPrefix = "Array<";
    if (resolved.size() > arrayPrefix.size() && resolved.substr(0, arrayPrefix.size()) == arrayPrefix
        && resolved.back() == '>')
    {
        RbScriptType type = MakeType(RbScriptTypeKind::Array, resolved.c_str());
        type.elementType = resolved.substr(arrayPrefix.size(), resolved.size() - arrayPrefix.size() - 1);
        return type;
    }

    const ea::string optionalPrefix = "Optional<";
    if (resolved.size() > optionalPrefix.size() && resolved.substr(0, optionalPrefix.size()) == optionalPrefix
        && resolved.back() == '>')
    {
        RbScriptType type = MakeType(RbScriptTypeKind::Optional, resolved.c_str());
        type.elementType = resolved.substr(optionalPrefix.size(), resolved.size() - optionalPrefix.size() - 1);
        return type;
    }

    const ea::string mapPrefix = "Map<";
    if (resolved.size() > mapPrefix.size() && resolved.substr(0, mapPrefix.size()) == mapPrefix
        && resolved.back() == '>')
    {
        RbScriptType type = MakeType(RbScriptTypeKind::Map, resolved.c_str());
        const unsigned comma = resolved.find(',');
        if (comma != ea::string::npos)
        {
            type.keyType = Trim(resolved.substr(mapPrefix.size(), comma - mapPrefix.size()));
            type.elementType = Trim(resolved.substr(comma + 1, resolved.size() - comma - 2));
        }
        return type;
    }

    return {};
}

const RbScriptFunctionSignature* RbScriptTypeRegistry::FindFunction(const ea::string& name) const
{
    const auto found = functions_.find(name);
    return found != functions_.end() ? &found->second : nullptr;
}

bool RbScriptTypeRegistry::HasType(const ea::string& name) const
{
    return Resolve(name).IsValid();
}

RbScriptTypeChecker::RbScriptTypeChecker(const RbScriptTypeRegistry& registry)
    : registry_(registry)
{
}

bool RbScriptTypeChecker::Check(const RbScriptModule& module)
{
    diagnostics_.clear();
    symbols_.clear();
    functions_.clear();

    for (const RbScriptScript& script : module.scripts)
    {
        for (const RbScriptFunction& function : script.functions)
        {
            RbScriptFunctionSignature signature;
            signature.name = function.name;
            signature.returnType = registry_.Resolve(function.returnType);
            signature.asynchronous = function.asynchronous;
            for (const RbScriptParameter& parameter : function.parameters)
                signature.parameterTypes.push_back(registry_.Resolve(parameter.typeName));
            functions_[function.name] = signature;
        }
    }

    for (const RbScriptScript& script : module.scripts)
        CheckScript(script);

    for (const RbScriptDiagnostic& diagnostic : diagnostics_)
    {
        if (diagnostic.severity == RbScriptDiagnosticSeverity::Error)
            return false;
    }
    return true;
}

void RbScriptTypeChecker::AddDiagnostic(RbScriptDiagnosticSeverity severity, const ea::string& code,
    const ea::string& message, const RbScriptSourceSpan& span)
{
    RbScriptDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.span = span;
    diagnostics_.push_back(diagnostic);
}

void RbScriptTypeChecker::CheckScript(const RbScriptScript& script)
{
    symbols_.clear();
    for (const RbScriptField& field : script.fields)
    {
        const RbScriptType type = ResolveOrReport(field.typeName, field.span);
        symbols_[field.name] = type;
        if (field.initializer)
        {
            const RbScriptType value = InferExpression(*field.initializer);
            if (!type.IsAssignableFrom(value))
                AddDiagnostic(RbScriptDiagnosticSeverity::Error, "T1001",
                    "Field initializer type '" + value.ToString() + "' is not assignable to '" + type.ToString() + "'", field.span);
        }
    }

    for (const RbScriptFunction& function : script.functions)
        CheckFunction(function);
}

void RbScriptTypeChecker::CheckFunction(const RbScriptFunction& function)
{
    const RbScriptType returnType = ResolveOrReport(function.returnType, function.span);
    for (const RbScriptParameter& parameter : function.parameters)
        symbols_[parameter.name] = ResolveOrReport(parameter.typeName, parameter.span);
    for (const std::unique_ptr<RbScriptStatement>& statement : function.body)
        CheckStatement(*statement, returnType);
}

void RbScriptTypeChecker::CheckStatement(const RbScriptStatement& statement, const RbScriptType& expectedReturn)
{
    switch (statement.kind)
    {
    case RbScriptStatementKind::Block:
        for (const std::unique_ptr<RbScriptStatement>& child : statement.body)
            CheckStatement(*child, expectedReturn);
        break;

    case RbScriptStatementKind::VariableDeclaration:
    {
        const RbScriptType type = ResolveOrReport(statement.typeName, statement.span);
        symbols_[statement.name] = type;
        if (statement.expression)
        {
            const RbScriptType value = InferExpression(*statement.expression);
            if (!type.IsAssignableFrom(value))
                AddDiagnostic(RbScriptDiagnosticSeverity::Error, "T1002",
                    "Initializer type '" + value.ToString() + "' is not assignable to '" + type.ToString() + "'", statement.span);
        }
        break;
    }

    case RbScriptStatementKind::Return:
        if (statement.expression)
        {
            const RbScriptType value = InferExpression(*statement.expression);
            if (!expectedReturn.IsAssignableFrom(value))
                AddDiagnostic(RbScriptDiagnosticSeverity::Error, "T1003",
                    "Return type '" + value.ToString() + "' is not assignable to '" + expectedReturn.ToString() + "'", statement.span);
        }
        else if (expectedReturn.kind != RbScriptTypeKind::Void)
            AddDiagnostic(RbScriptDiagnosticSeverity::Error, "T1004", "Non-void function must return a value", statement.span);
        break;

    case RbScriptStatementKind::If:
    case RbScriptStatementKind::While:
    {
        if (statement.expression && InferExpression(*statement.expression).kind != RbScriptTypeKind::Bool)
            AddDiagnostic(RbScriptDiagnosticSeverity::Error, "T1005", "Condition must have type bool", statement.span);
        for (const std::unique_ptr<RbScriptStatement>& child : statement.body)
            CheckStatement(*child, expectedReturn);
        for (const std::unique_ptr<RbScriptStatement>& child : statement.elseBody)
            CheckStatement(*child, expectedReturn);
        break;
    }

    default:
        if (statement.expression)
            InferExpression(*statement.expression);
        for (const std::unique_ptr<RbScriptStatement>& child : statement.body)
            CheckStatement(*child, expectedReturn);
        break;
    }
}

RbScriptType RbScriptTypeChecker::InferExpression(const RbScriptExpression& expression)
{
    switch (expression.kind)
    {
    case RbScriptExpressionKind::IntegerLiteral: return registry_.Resolve("i32");
    case RbScriptExpressionKind::FloatLiteral: return registry_.Resolve("f32");
    case RbScriptExpressionKind::StringLiteral: return registry_.Resolve("String");
    case RbScriptExpressionKind::BooleanLiteral: return registry_.Resolve("bool");
    case RbScriptExpressionKind::NullLiteral: return registry_.Resolve("Variant");

    case RbScriptExpressionKind::Identifier:
    {
        const auto symbol = symbols_.find(expression.text);
        if (symbol != symbols_.end())
            return symbol->second;
        const auto function = functions_.find(expression.text);
        if (function != functions_.end())
            return function->second.returnType;
        return ResolveOrReport(expression.text, expression.span);
    }

    case RbScriptExpressionKind::Unary:
        return expression.children.empty() ? RbScriptType{} : InferExpression(*expression.children.front());

    case RbScriptExpressionKind::Member:
        return registry_.Resolve("Variant");

    case RbScriptExpressionKind::Call:
        if (!expression.children.empty())
        {
            const RbScriptExpression& callee = *expression.children.front();
            if (callee.kind == RbScriptExpressionKind::Identifier)
            {
                const auto function = functions_.find(callee.text);
                if (function != functions_.end())
                    return function->second.returnType;
                const RbScriptFunctionSignature* reflected = registry_.FindFunction(callee.text);
                if (reflected)
                    return reflected->returnType;
            }
            for (unsigned i = 1; i < expression.children.size(); ++i)
                InferExpression(*expression.children[i]);
        }
        return registry_.Resolve("Variant");

    case RbScriptExpressionKind::Binary:
    {
        if (expression.children.size() < 2)
            return registry_.Resolve("Variant");
        const RbScriptType left = InferExpression(*expression.children[0]);
        const RbScriptType right = InferExpression(*expression.children[1]);
        if (IsLogical(expression.text) || IsComparison(expression.text))
            return registry_.Resolve("bool");
        if (expression.text == "+" && left.kind == RbScriptTypeKind::String && right.kind == RbScriptTypeKind::String)
            return left;
        if (left.IsNumeric() && right.IsNumeric())
            return left.kind == RbScriptTypeKind::Double || right.kind == RbScriptTypeKind::Double
                ? registry_.Resolve("f64") : left.kind == RbScriptTypeKind::Float || right.kind == RbScriptTypeKind::Float
                ? registry_.Resolve("f32") : left;
        return registry_.Resolve("Variant");
    }

    default:
        return registry_.Resolve("Variant");
    }
}

RbScriptType RbScriptTypeChecker::ResolveOrReport(const ea::string& name, const RbScriptSourceSpan& span)
{
    const RbScriptType type = registry_.Resolve(name);
    if (type.IsValid())
        return type;
    AddDiagnostic(RbScriptDiagnosticSeverity::Error, "T1000", "Unknown type or symbol '" + name + "'", span);
    return registry_.Resolve("Variant");
}

} // namespace Urho3D
