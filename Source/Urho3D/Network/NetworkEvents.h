//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// Server connection established.
URHO3D_EVENT(E_SERVERCONNECTED, ServerConnected)
{
}

/// Server connection disconnected.
URHO3D_EVENT(E_SERVERDISCONNECTED, ServerDisconnected)
{
}

/// Server connection failed.
URHO3D_EVENT(E_CONNECTFAILED, ConnectFailed)
{
}

/// Server connection failed because its already connected or tries to connect already.
URHO3D_EVENT(E_CONNECTIONINPROGRESS, ConnectionInProgress)
{
}

/// New client connection established.
URHO3D_EVENT(E_CLIENTCONNECTED, ClientConnected)
{
    URHO3D_PARAM(P_CONNECTION, Connection);        // Connection pointer
}

/// Client connection disconnected.
URHO3D_EVENT(E_CLIENTDISCONNECTED, ClientDisconnected)
{
    URHO3D_PARAM(P_CONNECTION, Connection);        // Connection pointer
}

/// Client has sent identity: identity map is in the event data.
URHO3D_EVENT(E_CLIENTIDENTITY, ClientIdentity)
{
    URHO3D_PARAM(P_CONNECTION, Connection);        // Connection pointer
    URHO3D_PARAM(P_ALLOW, Allow);                  // bool
}

/// Client has informed to have loaded the scene.
URHO3D_EVENT(E_CLIENTSCENELOADED, ClientSceneLoaded)
{
    URHO3D_PARAM(P_CONNECTION, Connection);        // Connection pointer
}

/// Unhandled network message received.
URHO3D_EVENT(E_NETWORKMESSAGE, NetworkMessage)
{
    URHO3D_PARAM(P_CONNECTION, Connection);        // Connection pointer
    URHO3D_PARAM(P_MESSAGEID, MessageID);          // int
    URHO3D_PARAM(P_DATA, Data);                    // Buffer
}

/// Incoming network messages are processed on the client or server.
URHO3D_EVENT(E_NETWORKINPUTPROCESSED, NetworkInputProcessed)
{
    URHO3D_PARAM(P_TIMESTEP, TimeStep);                 // float
}

/// Begin the frame of network update on the server.
URHO3D_EVENT(E_BEGINSERVERNETWORKFRAME, BeginServerNetworkFrame)
{
    URHO3D_PARAM(P_FRAME, Frame);                       // long long
}

/// End the frame of network update on the server. Happens right before sending messages to the clients.
URHO3D_EVENT(E_ENDSERVERNETWORKFRAME, EndServerNetworkFrame)
{
    URHO3D_PARAM(P_FRAME, Frame);                       // long long
}

/// Begin the input frame on the client.
/// Any client-side simulation at E_BEGINCLIENTNETWORKFRAME is precisely synchronized
/// with one of the future E_BEGINSERVERNETWORKFRAME.
URHO3D_EVENT(E_BEGINCLIENTNETWORKFRAME, BeginClientNetworkFrame)
{
    URHO3D_PARAM(P_FRAME, Frame);                       // long long
}

/// End the input frame on the client. Happens right before sending messages to the server.
URHO3D_EVENT(E_ENDCLIENTNETWORKFRAME, EndClientNetworkFrame)
{
    URHO3D_PARAM(P_FRAME, Frame);                       // long long
}

/// About to send network update on the client or server.
URHO3D_EVENT(E_NETWORKUPDATE, NetworkUpdate)
{
    URHO3D_PARAM(P_ISSERVER, IsServer);                 // bool
}

/// Network update has been sent on the client or server.
URHO3D_EVENT(E_NETWORKUPDATESENT, NetworkUpdateSent)
{
    URHO3D_PARAM(P_ISSERVER, IsServer);                 // bool
}

/// Scene load failed, either due to file not found or checksum error.
URHO3D_EVENT(E_NETWORKSCENELOADFAILED, NetworkSceneLoadFailed)
{
    URHO3D_PARAM(P_CONNECTION, Connection);      // Connection pointer
}

/// Remote event: adds Connection parameter to the event data.
URHO3D_EVENT(E_REMOTEEVENTDATA, RemoteEventData)
{
    URHO3D_PARAM(P_CONNECTION, Connection);      // Connection pointer
}

/// When LAN discovery found hosted server.
URHO3D_EVENT(E_NETWORKHOSTDISCOVERED, NetworkHostDiscovered)
{
    URHO3D_PARAM(P_ADDRESS, Address);   // String
    URHO3D_PARAM(P_PORT, Port);         // int
    URHO3D_PARAM(P_BEACON, Beacon);     // VariantMap
}

}
