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

#include "../Math/MathDefs.h"

namespace Urho3D
{

enum NetworkMessageId
{
    /// Client->server: send VariantMap of identity and authentication data.
    MSG_IDENTITY = 0x87,
    /// Client->server: send controls (buttons and mouse movement).
    MSG_CONTROLS = 0x88,
    /// Client->server: scene has been loaded and client is ready to proceed.
    MSG_SCENELOADED = 0x89,
    /// Client->server: request a package file.
    MSG_REQUESTPACKAGE = 0x8A,

    /// Server->client: package file data fragment.
    MSG_PACKAGEDATA = 0x8B,
    /// Server->client: load new scene. In case of empty filename the client should just empty the scene.
    MSG_LOADSCENE = 0x8C,
    /// Server->client: wrong scene checksum, can not participate.
    MSG_SCENECHECKSUMERROR = 0x8D,
    /// Server->client: create new node.
    MSG_CREATENODE = 0x8E,
    /// Server->client: node delta update.
    MSG_NODEDELTAUPDATE = 0x8F,
    /// Server->client: node latest data update.
    MSG_NODELATESTDATA = 0x90,
    /// Server->client: remove node.
    MSG_REMOVENODE = 0x91,
    /// Server->client: create new component.
    MSG_CREATECOMPONENT = 0x92,
    /// Server->client: component delta update.
    MSG_COMPONENTDELTAUPDATE = 0x93,
    /// Server->client: component latest data update.
    MSG_COMPONENTLATESTDATA = 0x94,
    /// Server->client: remove component.
    MSG_REMOVECOMPONENT = 0x95,

    /// Client->server and server->client: remote event.
    MSG_REMOTEEVENT = 0x96,
    /// Client->server and server->client: remote node event.
    MSG_REMOTENODEEVENT = 0x97,
    /// Server->client: info about package.
    MSG_PACKAGEINFO = 0x98,

    /// Packet that includes all other messages.
    MSG_PACKED_MESSAGE = 0x99,

    /// Message used to synchronize clock between client and server.
    MSG_CLOCK_SYNC = 0x9A,

    /// Server->Client. ReplicationManager message. Deliver networking settings.
    MSG_CONFIGURE = 200,
    /// Server->Client. ReplicationManager message. Send server time and dynamic properties of the client connection.
    MSG_SCENE_CLOCK,
    /// Client->Server. ReplicationManager message. Notify server that the client is ready for replication.
    MSG_SYNCHRONIZED,
    /// Server->Client. ReplicationManager message. Remove replicated NetworkObjects.
    MSG_REMOVE_OBJECTS,
    /// Server->Client. ReplicationManager message. Create replicated NetworkObjects from snapshots.
    MSG_ADD_OBJECTS,
    /// Server->Client. ReplicationManager message. Perform ordered and reliable update of NetworkObjects.
    MSG_UPDATE_OBJECTS_RELIABLE,
    /// Server->Client. ReplicationManager message. Perform unordered and unreliable update of NetworkObjects.
    MSG_UPDATE_OBJECTS_UNRELIABLE,
    /// Client->Server. ReplicationManager message. Perform unordered and unreliable update of owned NetworkObjects from client to server.
    MSG_OBJECTS_FEEDBACK_UNRELIABLE,

    /// Message IDs starting from MSG_USER are reserved for the end user.
    MSG_USER = 512
};

/// Package file fragment size.
static const unsigned PACKAGE_FRAGMENT_SIZE = 1024;

}
