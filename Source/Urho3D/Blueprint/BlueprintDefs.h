// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Container/Str.h>
#include <EASTL/vector.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Math/Vector2.h>

namespace Urho3D
{

/// Stable identifier used by a Blueprint graph.
using BlueprintId = unsigned;

/// Invalid Blueprint identifier.
static constexpr BlueprintId BLUEPRINT_INVALID_ID = 0;

/// Pin direction and semantic role.
enum class BlueprintPinKind
{
    Input,
    Output,
    ExecutionInput,
    ExecutionOutput
};

/// Data types exposed by the built-in Blueprint nodes.
enum class BlueprintDataType
{
    Wildcard,
    Bool,
    Int,
    Int64,
    Float,
    Double,
    String,
    Vector2,
    Vector3,
    Vector4,
    Color,
    Quaternion,
    Entity,
    Object,
    Variant
};

/// Severity of a graph validation diagnostic.
enum class BlueprintDiagnosticSeverity
{
    Info,
    Warning,
    Error
};

/// Execution model of a node.
enum class BlueprintExecutionMode
{
    Pure,
    Immediate,
    Latent
};

/// A diagnostic generated while validating or compiling a graph.
struct URHO3D_API BlueprintDiagnostic
{
    BlueprintDiagnosticSeverity severity{BlueprintDiagnosticSeverity::Info};
    BlueprintId nodeId{BLUEPRINT_INVALID_ID};
    ea::string code;
    ea::string message;
};

/// A pin on a Blueprint node.
struct URHO3D_API BlueprintPin
{
    ea::string name;
    ea::string displayName;
    BlueprintPinKind kind{BlueprintPinKind::Input};
    BlueprintDataType dataType{BlueprintDataType::Wildcard};
    Variant defaultValue;
    bool required{false};
    bool allowMultipleConnections{false};
};

/// A node instance stored in a Blueprint graph.
struct URHO3D_API BlueprintNode
{
    BlueprintId id{BLUEPRINT_INVALID_ID};
    ea::string typeName;
    ea::string title;
    ea::string category;
    Vector2 position{Vector2::ZERO};
    BlueprintExecutionMode executionMode{BlueprintExecutionMode::Immediate};
    bool enabled{true};
    ea::vector<BlueprintPin> pins;
    StringVariantMap properties;
};

/// A directed connection between two pins.
struct URHO3D_API BlueprintLink
{
    BlueprintId id{BLUEPRINT_INVALID_ID};
    BlueprintId fromNode{BLUEPRINT_INVALID_ID};
    ea::string fromPin;
    BlueprintId toNode{BLUEPRINT_INVALID_ID};
    ea::string toPin;
};

/// A user-defined graph variable.
struct URHO3D_API BlueprintVariable
{
    ea::string name;
    BlueprintDataType dataType{BlueprintDataType::Variant};
    Variant defaultValue;
    bool exposeOnInstance{false};
};

/// Result of graph validation.
struct URHO3D_API BlueprintValidationResult
{
    ea::vector<BlueprintDiagnostic> diagnostics;

    bool HasErrors() const;
    bool IsValid() const;
};

/// Convert an enum value to the stable serialized spelling used by .blueprint files.
URHO3D_API ea::string ToString(BlueprintPinKind value);
URHO3D_API ea::string ToString(BlueprintDataType value);
URHO3D_API ea::string ToString(BlueprintDiagnosticSeverity value);
URHO3D_API ea::string ToString(BlueprintExecutionMode value);

/// Parse a stable serialized spelling. Unknown values fall back to a safe default.
URHO3D_API BlueprintPinKind ParseBlueprintPinKind(const ea::string& value);
URHO3D_API BlueprintDataType ParseBlueprintDataType(const ea::string& value);
URHO3D_API BlueprintDiagnosticSeverity ParseBlueprintDiagnosticSeverity(const ea::string& value);
URHO3D_API BlueprintExecutionMode ParseBlueprintExecutionMode(const ea::string& value);

}
