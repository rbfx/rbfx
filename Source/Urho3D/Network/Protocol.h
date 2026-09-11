// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <cstdint>

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
/// @see ReplicatedPeer::GetMaxMessageSize
static constexpr unsigned MaxNetworkPacketSize = 1024;
/// Maximum possible size of network message payload. Transport limitations are not considered.
/// Real limit may be lower.
/// @see ReplicatedPeer::GetMaxMessageSize
static constexpr unsigned MaxNetworkMessageSize = 0xFFFF;

/// Max size of the message header (message id only).
static constexpr unsigned NetworkMessageHeaderSize = 4;

} // namespace Urho3D
