// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Replica/ProtocolMessages.h"

#include "../Core/StringUtils.h"
#include "../Replica/ReplicationManager.h"

namespace Urho3D
{

void MsgConfigure::Save(VectorBuffer& dest) const
{
    dest.WriteUInt(magic_);
    dest.WriteVariantMap(settings_);
}

void MsgConfigure::Load(MemoryBuffer& src)
{
    magic_ = src.ReadUInt();
    settings_ = src.ReadVariantMap();
}

ea::string MsgConfigure::ToString() const
{
    return Format("{{magic={}, settings...}}", magic_);
}


void MsgSynchronized::Save(VectorBuffer& dest) const
{
    dest.WriteUInt(magic_);
}

void MsgSynchronized::Load(MemoryBuffer& src)
{
    magic_ = src.ReadUInt();
}

ea::string MsgSynchronized::ToString() const
{
    return Format("{{magic={}}}", magic_);
}

void MsgSceneClock::Save(VectorBuffer& dest) const
{
    dest.WriteInt64(static_cast<long long>(latestFrame_));
    dest.WriteUInt(latestFrameTime_);
    dest.WriteVLE(inputDelay_);
}

void MsgSceneClock::Load(MemoryBuffer& src)
{
    latestFrame_ = static_cast<NetworkFrame>(src.ReadInt64());
    latestFrameTime_ = src.ReadUInt();
    inputDelay_ = src.ReadVLE();
}

ea::string MsgSceneClock::ToString() const
{
    return Format("{{latestFrame={} at {}, inputDelay={}}}", latestFrame_, latestFrameTime_, inputDelay_);
}

}
