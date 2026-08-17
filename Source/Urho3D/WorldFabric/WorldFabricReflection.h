// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/WorldFabric/WorldFabric.h>

namespace Urho3D
{

class Context;
class RbScriptTypeRegistry;

/// Projects rbfx reflection and rbscript signatures into the World Fabric graph.
class URHO3D_API WorldFabricReflection
{
public:
    /// Register reflected rbfx object types and their editable properties.
    /// Returns the number of semantic nodes created or updated.
    static unsigned RegisterObjectReflection(Context* context, WorldFabricGraph& graph);
    /// Register rbscript user types and functions already present in the registry.
    /// Returns the number of semantic nodes created or updated.
    static unsigned RegisterRbScriptReflection(const RbScriptTypeRegistry& registry, WorldFabricGraph& graph);
};

} // namespace Urho3D
