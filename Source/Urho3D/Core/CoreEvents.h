// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

/// Frame begin event.
URHO3D_EVENT(E_BEGINFRAME, BeginFrame)
{
    URHO3D_PARAM(P_FRAMENUMBER, FrameNumber);      // unsigned
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Pre-update event that indicates that both local and network input has been processed.
URHO3D_EVENT(E_INPUTREADY, InputReady)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Application-wide logic update event.
URHO3D_EVENT(E_UPDATE, Update)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Application-wide logic post-update event.
URHO3D_EVENT(E_POSTUPDATE, PostUpdate)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Render update event.
URHO3D_EVENT(E_RENDERUPDATE, RenderUpdate)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Post-render update event.
URHO3D_EVENT(E_POSTRENDERUPDATE, PostRenderUpdate)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Frame end event.
URHO3D_EVENT(E_ENDFRAME, EndFrame)
{
}

/// Frame end event, only for tools and testing.
URHO3D_EVENT(E_ENDFRAMEPRIVATE, EndFramePrivate)
{
}

}
