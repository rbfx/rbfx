// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

namespace Urho3D
{
// clang-format off

/// IKSolver at the sender Node is about to start solving.
URHO3D_EVENT(E_IKPRESOLVE, IKPreSolve)
{
    URHO3D_PARAM(P_NODE, Node);         // Node pointer
    URHO3D_PARAM(P_IKSOLVER, IKSolver); // IKSolver pointer
}

/// IKSolver at the sender Node has finished solving.
URHO3D_EVENT(E_IKPOSTSOLVE, IKPostSolve)
{
    URHO3D_PARAM(P_NODE, Node);         // Node pointer
    URHO3D_PARAM(P_IKSOLVER, IKSolver); // IKSolver pointer
}

// clang-format on
} // namespace Urho3D
