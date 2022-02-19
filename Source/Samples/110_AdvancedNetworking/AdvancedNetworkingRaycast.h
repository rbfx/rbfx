//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022 the rbfx project.
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

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Replica/NetworkTime.h>

using namespace Urho3D;

/// Server-side raycast info to be processed.
struct ServerRaycastInfo
{
    WeakPtr<Connection> clientConnection_;
    Vector3 origin_;
    Vector3 target_;
    NetworkTime replicaTime_;
    NetworkTime inputTime_;
};

URHO3D_EVENT(E_ADVANCEDNETWORKING_RAYCAST, AdvancedNetworkingRaycast)
{
    URHO3D_PARAM(P_ORIGIN, Origin);
    URHO3D_PARAM(P_TARGET, Target);
    URHO3D_PARAM(P_REPLICA_FRAME, ReplicaFrame);
    URHO3D_PARAM(P_REPLICA_SUBFRAME, ReplicaSubFrame);
    URHO3D_PARAM(P_INPUT_FRAME, InputFrame);
    URHO3D_PARAM(P_INPUT_SUBFRAME, InputSubFrame);
}

URHO3D_EVENT(E_ADVANCEDNETWORKING_RAYHIT, AdvancedNetworkingRayhit)
{
    URHO3D_PARAM(P_ORIGIN, Origin);
    URHO3D_PARAM(P_POSITION, Position);
}


