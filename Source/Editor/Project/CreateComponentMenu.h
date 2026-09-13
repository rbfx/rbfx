// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Context.h>

#include <EASTL/optional.h>

namespace Urho3D
{

class ObjectReflection;

/// Helper function to render "create component menu".
ObjectReflection* RenderCreateComponentMenu(Context* context);

}
