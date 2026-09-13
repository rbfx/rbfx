// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

namespace Urho3D
{

URHO3D_EVENT(E_STATETRANSITIONSTARTED, StateTransitionStarted)
{
    URHO3D_PARAM(P_FROM, From); // (StringHash) Origin state type hash
    URHO3D_PARAM(P_TO, To); // (StringHash) Destination state type hash
}

URHO3D_EVENT(E_LEAVINGAPPLICATIONSTATE, LeavingApplicationState)
{
    URHO3D_PARAM(P_FROM, From); // (StringHash) Origin state type hash
    URHO3D_PARAM(P_TO, To); // (StringHash) Destination state type hash
}

URHO3D_EVENT(E_ENTERINGAPPLICATIONSTATE, EnteringApplicationState)
{
    URHO3D_PARAM(P_FROM, From); // (StringHash) Origin state type hash
    URHO3D_PARAM(P_TO, To); // (StringHash) Destination state type hash
}

URHO3D_EVENT(E_STATETRANSITIONCOMPLETE, StateTransitionComplete)
{
    URHO3D_PARAM(P_FROM, From); // (StringHash) Origin state type hash
    URHO3D_PARAM(P_TO, To); // (StringHash) Destination state type hash
}

} // namespace Urho3D
