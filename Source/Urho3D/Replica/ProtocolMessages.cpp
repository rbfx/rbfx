//
// Copyright (c) 2017-2020 the rbfx project.
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
