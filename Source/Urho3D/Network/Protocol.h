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

#include <stdint.h>

namespace Urho3D
{

/// Known message IDs used by networking system. User-defined message IDs should start from MSG_USER.
/// Expect values of these message IDs to change between versions.
enum NetworkMessageId : uint32_t
{
    /// Message used to synchronize clock between client and server.
    MSG_CLOCK_SYNC,

    /// Server->Client. ReplicationManager message. Deliver networking settings.
    MSG_CONFIGURE,
    /// Server->Client. ReplicationManager message. Send server time and dynamic properties of the client connection.
    MSG_SCENE_CLOCK,
    /// Client->Server. ReplicationManager message. Notify server that the client is ready for replication.
    MSG_SYNCHRONIZED,
    /// Server->Client. ReplicationManager message. Remove replicated NetworkObjects.
    MSG_REMOVE_OBJECTS,
    /// Server->Client. ReplicationManager message. Create replicated NetworkObjects from snapshots.
    MSG_ADD_OBJECTS,
    MSG_ADD_OBJECTS_INCOMPLETE,
    /// Server->Client. ReplicationManager message. Perform ordered and reliable update of NetworkObjects.
    MSG_UPDATE_OBJECTS_RELIABLE,
    MSG_UPDATE_OBJECTS_RELIABLE_INCOMPLETE,
    /// Server->Client. ReplicationManager message. Perform unordered and unreliable update of NetworkObjects.
    MSG_UPDATE_OBJECTS_UNRELIABLE,
    /// Client->Server. ReplicationManager message. Perform unordered and unreliable update of owned NetworkObjects from client to server.
    MSG_OBJECTS_FEEDBACK_UNRELIABLE,

    /// Server->Client. Request client to load scene file and report result.
    MSG_LOAD_SCENE,
    /// Client->Server. Report scene loading result.
    MSG_SCENE_LOAD_RESULT,

    /// Message IDs starting from MSG_USER are reserved for the end user.
    MSG_USER,

    /// Max message ID value.
    MSG_MAX = 0xFFFFFFFF,
};

/// Conservative limit for the size of the packet transmitted over underlying transport.
/// Real limit may be higher.
/// @see ReplicatedPeer::GetMaxPacketSize
static constexpr unsigned MaxNetworkPacketSize = 1024;
/// Maximum possible size of network message payload. Transport limitations are not considered.
/// Real limit may be lower.
/// @see ReplicatedPeer::GetMaxMessageSize
static constexpr unsigned MaxNetworkMessageSize = 0xFFFF;

/// Max size of the message header (message id only).
static constexpr unsigned NetworkMessageHeaderSize = 5;

} // namespace Urho3D
