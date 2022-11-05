//
// Copyright (c) 2022 the RBFX project.
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
 
/// VR session has started.
URHO3D_EVENT(E_VRSESSIONSTART, VRSessionStart)
{

}

/// Paused VR session is being resumed, such as headset put back on.
URHO3D_EVENT(E_VRRESUME, VRResume)
{
}

/// VR session is being paused, such as headset sensor reporting removed.
URHO3D_EVENT(E_VRPAUSE, VRPause)
{
    URHO3D_PARAM(P_STATE, State);   // bool
}

/// An external source is terminating our VR instance.
URHO3D_EVENT(E_VREXIT, VRExit)
{

}

/// Interaction profile has been changed, which means bindings have been remapped.
URHO3D_EVENT(E_VRINTERACTIONPROFILECHANGED, VRInteractionProfileChanged)
{
}

/// An input binding has been changed.
URHO3D_EVENT(E_BINDINGCHANGED, VRBindingChange)
{
    URHO3D_PARAM(P_NAME, Name);     // String
    URHO3D_PARAM(P_DATA, Data);     // Variant
    URHO3D_PARAM(P_DELTA, Delta);  // Variant
    URHO3D_PARAM(P_EXTRADELTA, ExtraDelta); // Variant
    URHO3D_PARAM(P_ACTIVE, Active);     // bool
    URHO3D_PARAM(P_BINDING, Binding);   // Binding pointer
}

/// Controller model has been changed.
URHO3D_EVENT(E_VRCONTROLLERCHANGE, VRControllerChange)
{
    URHO3D_PARAM(P_HAND, Hand); // int
}

URHO3D_EVENT(E_VRPUSH, VRPush)
{
    URHO3D_PARAM(P_BODY, Body);     // RigidBody pointer
    URHO3D_PARAM(P_NORMAL, Normal); // Vector3
}

/// Ongoing fall.
URHO3D_EVENT(E_VRFALLING, VRFalling)
{
    URHO3D_PARAM(P_FALLTIME, Falltime); // float
}

/// Finished falling and contacted a flat surface.
URHO3D_EVENT(E_VRLANDED, VRLanded)
{
    URHO3D_PARAM(P_BODY, Body);         // RigidBody pointer
    URHO3D_PARAM(P_FALLTIME, Falltime); // float
}

/// Hit a surface we cannot step over, typically a wall.
URHO3D_EVENT(E_VRHITWALL, VRHitwall)
{
    URHO3D_PARAM(P_BODY, Body);         // RigidBody pointer
    URHO3D_PARAM(P_NORMAL, Normal);     // Vector3
}
    
}
