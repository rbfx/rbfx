//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/Object.h"

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
