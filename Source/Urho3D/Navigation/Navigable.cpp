// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Navigation/Navigable.h"

#include "Urho3D/Core/Context.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

Navigable::Navigable(Context* context)
    : Component(context)
{
}

Navigable::~Navigable() = default;

void Navigable::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Navigable>(Category_Navigation);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Recursive", bool, recursive_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Walkable", bool, walkable_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Area ID", unsigned, areaId_, DeduceAreaId, AM_DEFAULT);
}

} // namespace Urho3D
