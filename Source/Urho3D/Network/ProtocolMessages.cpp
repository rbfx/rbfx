//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Network/ProtocolMessages.h"

#include "../Core/StringUtils.h"

namespace Urho3D
{

void MsgPingPong::Save(VectorBuffer& dest) const
{
    dest.WriteUInt(magic_);
}

void MsgPingPong::Load(MemoryBuffer& src)
{
    magic_ = src.ReadUInt();
}

ea::string MsgPingPong::ToString() const
{
    return Format("{{magic={}}}", magic_);
}

void MsgSynchronize::Save(VectorBuffer& dest) const
{
    dest.WriteUInt(magic_);

    dest.WriteVLE(updateFrequency_);
    dest.WriteVLE(numStartClockSamples_);
    dest.WriteVLE(numTrimmedClockSamples_);
    dest.WriteVLE(numOngoingClockSamples_);

    dest.WriteUInt(lastFrame_);
    dest.WriteVLE(ping_);
}

void MsgSynchronize::Load(MemoryBuffer& src)
{
    magic_ = src.ReadUInt();

    updateFrequency_ = src.ReadVLE();
    numStartClockSamples_ = src.ReadVLE();
    numTrimmedClockSamples_ = src.ReadVLE();
    numOngoingClockSamples_ = src.ReadVLE();

    lastFrame_ = src.ReadUInt();
    ping_ = src.ReadVLE();
}

ea::string MsgSynchronize::ToString() const
{
    return Format("{{magic={}, lastFrame={} ping={} updateFrequency={}}}",
        magic_, lastFrame_, ping_, updateFrequency_);
}

void MsgSynchronizeAck::Save(VectorBuffer& dest) const
{
    dest.WriteUInt(magic_);
}

void MsgSynchronizeAck::Load(MemoryBuffer& src)
{
    magic_ = src.ReadUInt();
}

ea::string MsgSynchronizeAck::ToString() const
{
    return Format("{{magic={}}}", magic_);
}

void MsgClock::Save(VectorBuffer& dest) const
{
    dest.WriteUInt(lastFrame_);
    dest.WriteVLE(ping_);
}

void MsgClock::Load(MemoryBuffer& src)
{
    lastFrame_ = src.ReadUInt();
    ping_ = src.ReadVLE();
}

ea::string MsgClock::ToString() const
{
    return Format("{{lastFrame={}, ping={}}}", lastFrame_, ping_);
}

}
