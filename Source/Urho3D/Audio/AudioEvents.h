// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

namespace Urho3D
{

/// Sound playback finished. Sent through the SoundSource's Node.
URHO3D_EVENT(E_SOUNDFINISHED, SoundFinished)
{
    URHO3D_PARAM(P_NODE, Node);                     // Node pointer
    URHO3D_PARAM(P_SOUNDSOURCE, SoundSource);       // SoundSource pointer
    URHO3D_PARAM(P_SOUND, Sound);                   // Sound pointer
}

URHO3D_EVENT(E_RECORDINGUPDATED, RecordingUpdated)
{
    URHO3D_PARAM(P_MICROPHONE, Microphone);         // Microphone pointer
    URHO3D_PARAM(P_DATALENGTH, DataLength);         // int
    URHO3D_PARAM(P_CLEARDATA, ClearData);           // bool
}

URHO3D_EVENT(E_RECORDINGSTARTED, RecordingStarted)
{
    URHO3D_PARAM(P_MICROPHONE, Microphone);         // Microphone pointer
}

URHO3D_EVENT(E_RECORDINGENDED, RecordingEnded)
{
    URHO3D_PARAM(P_MICROPHONE, Microphone);         // Microphone pointer
    URHO3D_PARAM(P_DATALENGTH, DataLength);         // int
    URHO3D_PARAM(P_CLEARDATA, ClearData);           // bool
}

}
