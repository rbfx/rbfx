// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Network/Protocol.h>
#include <Urho3D/Network/ReplicatedPeer.h>
#include <Urho3D/Network/Transport/NetworkConnection.h>
#include <Urho3D/Replica/NetworkTime.h>

using namespace Urho3D;

/// Server-side raycast info to be processed.
struct ServerRaycastInfo
{
    WeakPtr<NetworkConnection> clientConnection_;
    SharedPtr<ReplicatedPeer, RefCounted> clientPeer_;
    DoubleVector3 origin_;
    DoubleVector3 target_;
    NetworkTime replicaTime_;
    NetworkTime inputTime_;
};

static constexpr NetworkMessageId MSG_ADVANCEDNETWORKING_RAYCAST = static_cast<NetworkMessageId>(MSG_USER + 100);
static constexpr NetworkMessageId MSG_ADVANCEDNETWORKING_RAYHIT = static_cast<NetworkMessageId>(MSG_USER + 101);

inline void WriteNetworkTime(VectorBuffer& msg, const NetworkTime& time)
{
    msg.WriteInt64(static_cast<long long>(time.Frame()));
    msg.WriteFloat(time.Fraction());
}

inline NetworkTime ReadNetworkTime(MemoryBuffer& msg)
{
    return NetworkTime{static_cast<NetworkFrame>(msg.ReadInt64()), msg.ReadFloat()};
}

inline void WriteRaycastRequest(VectorBuffer& msg, const DoubleVector3& origin, const DoubleVector3& target,
    const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    msg.WritePackedVector3(origin, VectorBinaryEncoding::Double);
    msg.WritePackedVector3(target, VectorBinaryEncoding::Double);
    WriteNetworkTime(msg, replicaTime);
    WriteNetworkTime(msg, inputTime);
}

inline ServerRaycastInfo ReadRaycastRequest(NetworkConnection* connection, SharedPtr<ReplicatedPeer, RefCounted> peer,
    MemoryBuffer& message)
{
    ServerRaycastInfo info;
    info.clientConnection_ = connection;
    info.clientPeer_ = peer;
    info.origin_ = message.ReadPackedVector3(VectorBinaryEncoding::Double);
    info.target_ = message.ReadPackedVector3(VectorBinaryEncoding::Double);
    info.replicaTime_ = ReadNetworkTime(message);
    info.inputTime_ = ReadNetworkTime(message);
    return info;
}

inline void WriteRaycastResult(VectorBuffer& msg, const DoubleVector3& origin, const ea::optional<DoubleVector3>& hitPosition)
{
    msg.WritePackedVector3(origin, VectorBinaryEncoding::Double);
    msg.WriteBool(hitPosition.has_value());
    if (hitPosition)
        msg.WritePackedVector3(*hitPosition, VectorBinaryEncoding::Double);
}

inline ea::optional<DoubleVector3> ReadRaycastResult(MemoryBuffer& message)
{
    message.ReadPackedVector3(VectorBinaryEncoding::Double);
    if (!message.ReadBool())
        return ea::nullopt;
    return message.ReadPackedVector3(VectorBinaryEncoding::Double);
}


