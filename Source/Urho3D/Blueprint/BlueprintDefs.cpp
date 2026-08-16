// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintDefs.h"

namespace Urho3D
{

bool BlueprintValidationResult::HasErrors() const
{
    for (const BlueprintDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.severity == BlueprintDiagnosticSeverity::Error)
            return true;
    }
    return false;
}

bool BlueprintValidationResult::IsValid() const
{
    return !HasErrors();
}

ea::string ToString(BlueprintPinKind value)
{
    switch (value)
    {
    case BlueprintPinKind::Input: return "input";
    case BlueprintPinKind::Output: return "output";
    case BlueprintPinKind::ExecutionInput: return "exec_input";
    case BlueprintPinKind::ExecutionOutput: return "exec_output";
    default: return "input";
    }
}

ea::string ToString(BlueprintDataType value)
{
    switch (value)
    {
    case BlueprintDataType::Wildcard: return "wildcard";
    case BlueprintDataType::Bool: return "bool";
    case BlueprintDataType::Int: return "int";
    case BlueprintDataType::Int64: return "int64";
    case BlueprintDataType::Float: return "float";
    case BlueprintDataType::Double: return "double";
    case BlueprintDataType::String: return "string";
    case BlueprintDataType::Vector2: return "vector2";
    case BlueprintDataType::Vector3: return "vector3";
    case BlueprintDataType::Vector4: return "vector4";
    case BlueprintDataType::Color: return "color";
    case BlueprintDataType::Quaternion: return "quaternion";
    case BlueprintDataType::Entity: return "entity";
    case BlueprintDataType::Object: return "object";
    case BlueprintDataType::Variant: return "variant";
    case BlueprintDataType::Array: return "array";
    case BlueprintDataType::Map: return "map";
    case BlueprintDataType::Struct: return "struct";
    case BlueprintDataType::Enum: return "enum";
    default: return "variant";
    }
}

ea::string ToString(BlueprintDiagnosticSeverity value)
{
    switch (value)
    {
    case BlueprintDiagnosticSeverity::Info: return "info";
    case BlueprintDiagnosticSeverity::Warning: return "warning";
    case BlueprintDiagnosticSeverity::Error: return "error";
    default: return "info";
    }
}

ea::string ToString(BlueprintExecutionMode value)
{
    switch (value)
    {
    case BlueprintExecutionMode::Pure: return "pure";
    case BlueprintExecutionMode::Immediate: return "immediate";
    case BlueprintExecutionMode::Latent: return "latent";
    default: return "immediate";
    }
}

BlueprintPinKind ParseBlueprintPinKind(const ea::string& value)
{
    if (value == "output") return BlueprintPinKind::Output;
    if (value == "exec_input") return BlueprintPinKind::ExecutionInput;
    if (value == "exec_output") return BlueprintPinKind::ExecutionOutput;
    return BlueprintPinKind::Input;
}

BlueprintDataType ParseBlueprintDataType(const ea::string& value)
{
    if (value == "bool") return BlueprintDataType::Bool;
    if (value == "int") return BlueprintDataType::Int;
    if (value == "int64") return BlueprintDataType::Int64;
    if (value == "float") return BlueprintDataType::Float;
    if (value == "double") return BlueprintDataType::Double;
    if (value == "string") return BlueprintDataType::String;
    if (value == "vector2") return BlueprintDataType::Vector2;
    if (value == "vector3") return BlueprintDataType::Vector3;
    if (value == "vector4") return BlueprintDataType::Vector4;
    if (value == "color") return BlueprintDataType::Color;
    if (value == "quaternion") return BlueprintDataType::Quaternion;
    if (value == "entity") return BlueprintDataType::Entity;
    if (value == "object") return BlueprintDataType::Object;
    if (value == "array") return BlueprintDataType::Array;
    if (value == "map") return BlueprintDataType::Map;
    if (value == "struct") return BlueprintDataType::Struct;
    if (value == "enum") return BlueprintDataType::Enum;
    if (value == "wildcard") return BlueprintDataType::Wildcard;
    return BlueprintDataType::Variant;
}

BlueprintDiagnosticSeverity ParseBlueprintDiagnosticSeverity(const ea::string& value)
{
    if (value == "warning") return BlueprintDiagnosticSeverity::Warning;
    if (value == "error") return BlueprintDiagnosticSeverity::Error;
    return BlueprintDiagnosticSeverity::Info;
}

BlueprintExecutionMode ParseBlueprintExecutionMode(const ea::string& value)
{
    if (value == "pure") return BlueprintExecutionMode::Pure;
    if (value == "latent") return BlueprintExecutionMode::Latent;
    return BlueprintExecutionMode::Immediate;
}

}
