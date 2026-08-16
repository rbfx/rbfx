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
    Variant,
    Array,
    Map,
    Struct,
    Enum
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
    BlueprintId commentId{BLUEPRINT_INVALID_ID};
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

/// A movable comment box displayed behind nodes in the editor.
struct URHO3D_API BlueprintComment
{
    BlueprintId id{BLUEPRINT_INVALID_ID};
    ea::string text;
    Vector2 position{Vector2::ZERO};
    Vector2 size{Vector2{260.0f, 120.0f}};
    unsigned color{0x664A78A8};
};

/// Signature and serialized body of a user-defined Blueprint function/subgraph.
struct URHO3D_API BlueprintFunction
{
    ea::string name;
    ea::string description;
    ea::vector<BlueprintPin> inputs;
    ea::vector<BlueprintPin> outputs;
    ea::string body;
};

/// A named field in a user-defined Blueprint struct.
struct URHO3D_API BlueprintStructField
{
    ea::string name;
    BlueprintDataType dataType{BlueprintDataType::Variant};
    /// Optional user-defined type name for Struct and Enum fields.
    ea::string typeName;
    Variant defaultValue;
};

/// A user-defined value type composed of named fields.
struct URHO3D_API BlueprintStructDef
{
    ea::string name;
    ea::string description;
    ea::vector<BlueprintStructField> fields;
};

/// A named constant in a user-defined Blueprint enum.
struct URHO3D_API BlueprintEnumValue
{
    ea::string name;
    int value{0};
};

/// A user-defined enumeration of named integer constants.
struct URHO3D_API BlueprintEnumDef
{
    ea::string name;
    ea::string description;
    ea::vector<BlueprintEnumValue> values;
};

/// A typed callable signature used by Blueprint delegates and signals.
struct URHO3D_API BlueprintDelegate
{
    ea::string name;
    ea::string description;
    ea::vector<BlueprintPin> parameters;
};

/// A value keyframe used by a Blueprint timeline.
struct URHO3D_API BlueprintTimelineKeyframe
{
    float time{0.0f};
    Variant value;
};

/// A named timeline with editable keyframes and optional looping.
struct URHO3D_API BlueprintTimeline
{
    ea::string name;
    ea::string description;
    float length{1.0f};
    bool looping{false};
    ea::vector<BlueprintTimelineKeyframe> keyframes;
};

/// An inline Blueprint graph with macro semantics.
struct URHO3D_API BlueprintMacro
{
    ea::string name;
    ea::string description;
    ea::vector<BlueprintPin> inputs;
    ea::vector<BlueprintPin> outputs;
    ea::string body;
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
