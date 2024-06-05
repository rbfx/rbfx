// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Object.h>

namespace Urho3D
{
/// Module registered in container
URHO3D_EVENT(E_MODULEREGISTERED, ModuleRegistered)
{
    URHO3D_PARAM(P_CONTAINER, Container); // Container pointer
    URHO3D_PARAM(P_MODULE, Module); // Module component pointer
    URHO3D_PARAM(P_TYPE, Type); // Type under which the module was registered in container
}

/// Module removed (unregistered) from container
URHO3D_EVENT(E_MODULEREMOVED, ModuleRemoved)
{
    URHO3D_PARAM(P_CONTAINER, Container); // Container pointer
    URHO3D_PARAM(P_MODULE, Module); // Module component pointer
    URHO3D_PARAM(P_TYPE, Type); // Type under which the module was registered in container
}

}
