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

/// When LAN discovery found hosted server.
URHO3D_EVENT(E_NETWORKHOSTDISCOVERED, NetworkHostDiscovered)
{
    URHO3D_PARAM(P_ADDRESS, Address);   // String
    URHO3D_PARAM(P_PORT, Port);         // int
    URHO3D_PARAM(P_BEACON, Beacon);     // VariantMap
}

/// Client connection successfully established to a remote server.
URHO3D_EVENT(E_CLIENTCONNECTED, ClientConnected)
{
    URHO3D_PARAM(P_ADDRESS, Address);           // String
    URHO3D_PARAM(P_PORT, Port);                 // int
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
}

/// Client connection to a remote server has been terminated.
URHO3D_EVENT(E_CLIENTDISCONNECTED, ClientDisconnected)
{
    URHO3D_PARAM(P_ADDRESS, Address);           // String
    URHO3D_PARAM(P_PORT, Port);                 // int
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
}

/// A new client connection has been accepted by the server.
URHO3D_EVENT(E_SERVERCLIENTCONNECTED, ServerClientConnected)
{
    URHO3D_PARAM(P_SERVER, Server);             // NetworkServer ptr
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
    URHO3D_PARAM(P_ADDRESS, Address);           // String
    URHO3D_PARAM(P_PORT, Port);                 // int
}

/// A client connection has been disconnected from the server.
URHO3D_EVENT(E_SERVERCLIENTDISCONNECTED, ServerClientDisconnected)
{
    URHO3D_PARAM(P_SERVER, Server);             // NetworkServer ptr
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
    URHO3D_PARAM(P_ADDRESS, Address);           // String
    URHO3D_PARAM(P_PORT, Port);                 // int
}

/// Server has started listening for incoming connections.
URHO3D_EVENT(E_SERVERLISTENSTART, ServerListenStart)
{
    URHO3D_PARAM(P_SERVER, Server);             // NetworkServer ptr
}

/// Server has stopped listening for incoming connections.
URHO3D_EVENT(E_SERVERLISTENSTOP, ServerListenStop)
{
    URHO3D_PARAM(P_SERVER, Server);             // NetworkServer ptr
}

/// Raw data has been received on a network connection.
URHO3D_EVENT(E_CLIENTDATA, ClientData)
{
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
    URHO3D_PARAM(P_SIZE, Size);                 // int
    URHO3D_PARAM(P_DATA, Data);                 // const void*
    URHO3D_PARAM(P_HANDLED, Handled);           // bool
}

/// A network message has been received on a network connection.
URHO3D_EVENT(E_CLIENTMESSAGE, ClientMessage)
{
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
    URHO3D_PARAM(P_MESSAGEID, MessageID);       // int (NetworkMessageId)
    URHO3D_PARAM(P_SIZE, Size);                 // int
    URHO3D_PARAM(P_DATA, Data);                 // const void*
    URHO3D_PARAM(P_HANDLED, Handled);           // bool
}

/// Raw data is about to be sent on a network connection.
URHO3D_EVENT(E_CLIENTSENDDATA, ClientSendData)
{
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
    URHO3D_PARAM(P_TYPE, Type);                 // int (PacketTypeFlags)
    URHO3D_PARAM(P_SIZE, Size);                 // int
    URHO3D_PARAM(P_DATA, Data);                 // const void*
    URHO3D_PARAM(P_HANDLED, Handled);           // bool
}

/// A network message is about to be sent on a network connection.
URHO3D_EVENT(E_CLIENTSENDMESSAGE, ClientSendMessage)
{
    URHO3D_PARAM(P_CONNECTION, Connection);     // NetworkConnection ptr
    URHO3D_PARAM(P_MESSAGEID, MessageID);       // int (NetworkMessageId)
    URHO3D_PARAM(P_TYPE, Type);                 // int (PacketTypeFlags)
    URHO3D_PARAM(P_SIZE, Size);                 // int
    URHO3D_PARAM(P_DATA, Data);                 // const void*
    URHO3D_PARAM(P_HANDLED, Handled);           // bool
}

}
