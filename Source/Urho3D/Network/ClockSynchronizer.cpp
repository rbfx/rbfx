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

#include "../Network/ClockSynchronizer.h"

#include "../Core/Assert.h"
#include "../Core/Timer.h"
#include "../IO/Log.h"
#include "../Math/RandomEngine.h"

#include <cmath>

#include <EASTL/numeric.h>

namespace Urho3D
{

FilteredUint::FilteredUint(unsigned bufferSize, float maxDeviation)
    : maxDeviation_(maxDeviation)
{
    offsets_.set_capacity(bufferSize);
}

void FilteredUint::AddValue(unsigned value, bool filter)
{
    if (offsets_.empty())
        baseValue_ = value;

    const auto offset = static_cast<int>(value - baseValue_);
    offsets_.push_back(offset);

    if (filter)
        Filter();
}

void FilteredUint::Filter()
{
    if (offsets_.size() < 2)
    {
        const int offset = offsets_.empty() ? 0 : offsets_[0];
        averageOffset_ = offset;
        minOffset_ = offset;
        maxOffset_ = offset;
        stabilizedMaxAverageOffset_ = offset;
        return;
    }

    const double meanOffset = ea::accumulate(offsets_.begin(), offsets_.end(), 0.0) / offsets_.size();

    double varianceAccum = 0.0;
    for (int offset : offsets_)
        varianceAccum += (offset - meanOffset) * (offset - meanOffset);

    const double deviation = std::sqrt(varianceAccum) / (offsets_.size() - 1);

    double averageAccum = 0.0;
    unsigned averageCount = 0;
    minOffset_ = M_MAX_INT;
    maxOffset_ = M_MIN_INT;
    for (int offset : offsets_)
    {
        if (offset - meanOffset <= deviation * maxDeviation_)
        {
            averageAccum += offset;
            ++averageCount;

            minOffset_ = ea::min(minOffset_, offset);
            maxOffset_ = ea::max(maxOffset_, offset);
        }
    }

    URHO3D_ASSERT(averageCount > 0);
    averageOffset_ = static_cast<int>(averageAccum / averageCount);

    if (stabilizedMaxAverageOffset_ < averageOffset_)
        stabilizedMaxAverageOffset_ = averageOffset_;
    else if (stabilizedMaxAverageOffset_ > maxOffset_)
        stabilizedMaxAverageOffset_ = maxOffset_;
}

void ClockSynchronizerMessage::Load(Deserializer& src)
{
    magic_ = src.ReadUInt();
    phase_ = static_cast<ClockSynchronizerPhase>(src.ReadVLE());
    localSent_ = src.ReadUInt();
    remoteReceived_ = src.ReadUInt();
    remoteSent_ = src.ReadUInt();
}

void ClockSynchronizerMessage::Save(Serializer& dest) const
{
    dest.WriteUInt(magic_);
    dest.WriteVLE(static_cast<unsigned>(phase_));
    dest.WriteUInt(localSent_);
    dest.WriteUInt(remoteReceived_);
    dest.WriteUInt(remoteSent_);
}

ClockSynchronizer::ClockSynchronizer(unsigned pingIntervalMs, unsigned maxPingMs, unsigned clockBufferSize,
    unsigned pingBufferSize, ea::function<unsigned()> getTimestamp)
    : getTimestamp_(getTimestamp)
    , localToRemote_{clockBufferSize}
    , roundTripDelay_{pingBufferSize}
    , pingIntervalMs_{pingIntervalMs}
    , maxPingMs_{maxPingMs}
{
}

unsigned ClockSynchronizer::GetTimestamp() const
{
    return getTimestamp_ ? getTimestamp_() : Time::GetSystemTime();
}

void ClockSynchronizer::UpdateClocks(
    unsigned localSent, unsigned remoteReceived, unsigned remoteSent, unsigned localReceived)
{
    const unsigned offset1 = remoteReceived - localSent;
    const unsigned offset2 = remoteSent - localReceived;
    const auto delta = static_cast<int>(offset2 - offset1);
    const unsigned offset = offset1 + delta / 2;
    localToRemote_.AddValue(offset);

    const unsigned outerDelay = localReceived - localSent;
    const unsigned innerDelay = remoteSent - remoteReceived;
    const unsigned delay = outerDelay - ea::min(outerDelay, innerDelay);
    roundTripDelay_.AddValue(delay);
}

void ClockSynchronizer::ProcessMessage(const ClockSynchronizerMessage& msg)
{
    if (msg.phase_ == ClockSynchronizerPhase::Ping)
    {
        PendingPong pong;
        pong.magic_ = msg.magic_;
        pong.serverSentTime_ = msg.remoteSent_;
        pong.clientReceivedTime_ = GetTimestamp();
        pendingPongs_.push_back(pong);
    }
    else if (msg.phase_ == ClockSynchronizerPhase::Pong)
    {
        const auto isThisProbe = [&](const PendingPing& ping) { return ping.magic_ == msg.magic_; };
        const auto iter = ea::find_if(pendingPings_.begin(), pendingPings_.end(), isThisProbe);
        if (iter == pendingPings_.end())
        {
            URHO3D_LOGWARNING("Expired or invalid clock message was received");
            return;
        }

        const unsigned now = GetTimestamp();
        latestRoundtripTimestamp_ = now;
        UpdateClocks(iter->serverSentTime_, msg.remoteReceived_, msg.remoteSent_, now);
    }
}

ea::optional<ClockSynchronizerMessage> ClockSynchronizer::PollMessage()
{
    const unsigned now = GetTimestamp();
    if (!latestProbeTimestamp_ || (now - *latestProbeTimestamp_ >= pingIntervalMs_))
    {
        latestProbeTimestamp_ = now;
        CleanupExpiredPings(now);
        return CreateNewPing(now);
    }

    if (!pendingPongs_.empty())
    {
        const auto pong = pendingPongs_.back();
        pendingPongs_.pop_back();
        return CreateNewPong(pong);
    }

    return ea::nullopt;
}

ClockSynchronizerMessage ClockSynchronizer::CreateNewPing(unsigned now)
{
    PendingPing ping;
    ping.magic_ = RandomEngine::GetDefaultEngine().GetUInt();
    ping.serverSentTime_ = now;
    pendingPings_.push_back(ping);

    ClockSynchronizerMessage msg;
    msg.magic_ = ping.magic_;
    msg.phase_ = ClockSynchronizerPhase::Ping;
    msg.remoteSent_ = now;
    return msg;
}

ClockSynchronizerMessage ClockSynchronizer::CreateNewPong(const PendingPong& pong)
{
    ClockSynchronizerMessage msg;
    msg.magic_ = pong.magic_;
    msg.phase_ = ClockSynchronizerPhase::Pong;
    msg.localSent_ = pong.serverSentTime_;
    msg.remoteReceived_ = pong.clientReceivedTime_;
    msg.remoteSent_ = GetTimestamp();
    return msg;
}

void ClockSynchronizer::CleanupExpiredPings(unsigned now)
{
    if (pendingPings_.size() < 2 * maxPingMs_ / pingIntervalMs_)
        return;

    const auto isOutdated = [&](const PendingPing& ping) { return now - ping.serverSentTime_ >= maxPingMs_; };
    ea::erase_if(pendingPings_, isOutdated);
}

}
