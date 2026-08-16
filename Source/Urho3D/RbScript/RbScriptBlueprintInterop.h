// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptBindings.h"
#include "RbScriptVM.h"
#include <Urho3D/Blueprint/BlueprintRuntime.h>

namespace Urho3D
{

/// Bridges compiled rbscript functions and Blueprint calls while preserving rbfx Variant values.
class URHO3D_API RbScriptBlueprintInterop
{
public:
    static BlueprintDataType ToBlueprintDataType(const RbScriptType& type);
    static RbScriptValue FromVariant(const Variant& value);
    static Variant ToVariant(const RbScriptValue& value);

    /// Invoke a function from a compiled chunk using named Blueprint inputs.
    static bool Invoke(const RbScriptChunk& chunk, RbScriptVM& vm, const ea::string& functionName,
        const StringVariantMap& inputs, StringVariantMap& outputs);

    /// Register one Blueprint node for every [[blueprint_callable]] compiled function.
    static unsigned RegisterFunctionNodes(BlueprintRuntime& runtime, const RbScriptChunk& chunk, RbScriptVM& vm);

    /// Configure Blueprint -> rbscript execution for the generic Function.RbScript node.
    static void BindRuntime(BlueprintRuntime& runtime, const RbScriptChunk& chunk, RbScriptVM& vm);

    /// Configure rbscript blueprint::call to invoke functions in a Blueprint graph.
    static void BindBlueprintCalls(RbScriptBindings& bindings, BlueprintRuntime& runtime, const BlueprintGraph& graph);
};

} // namespace Urho3D
