// SPDX-License-Identifier: MIT

#pragma once

#include "BlueprintRuntime.h"

namespace Urho3D
{

class Context;

/// Generates Blueprint node definitions from rbfx ObjectReflection metadata.
class URHO3D_API BlueprintReflectionRegistry
{
public:
    /// Register getter and setter nodes for every supported reflected Serializable attribute.
    /// Returns the number of generated node definitions.
    static unsigned RegisterNodes(Context* context, BlueprintNodeRegistry& registry);

    /// Convert a rbfx VariantType to the closest Blueprint data type.
    static BlueprintDataType MapVariantType(VariantType type);
};

}
