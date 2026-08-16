// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptType.h"

#include <Urho3D/Core/Variant.h>

namespace Urho3D
{

class Context;

class URHO3D_API RbScriptReflection
{
public:
    static RbScriptType MapVariantType(VariantType type);
    static unsigned RegisterObjectReflection(Context* context, RbScriptTypeRegistry& registry);
};

} // namespace Urho3D
