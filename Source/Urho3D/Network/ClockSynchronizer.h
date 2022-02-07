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

/// \file

#pragma once

#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"

#include <EASTL/optional.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

/// Utility class to filter random uint value which mostly stays the same.
class URHO3D_API FilteredUint
{
public:
    explicit FilteredUint(unsigned bufferSize, float maxDeviation = 1.0f);

    void AddValue(unsigned value);
    void Filter();

    unsigned GetFilteredValue() const { return baseValue_ + filteredOffset_; }
    bool IsInitialized() const { return !offsets_.empty(); }

private:
    const float maxDeviation_{};

    unsigned baseValue_{};
    ea::ring_buffer<int> offsets_;
    int filteredOffset_{};
};

enum class ClockSynchronizerPhase : unsigned
{
    /// Phase 1, Server->client.
    /// Payload is relative to the server. Only "local sent" time is filled.
    Ping = 1,
    /// Phase 2, Client->server.
    /// Payload is relative to the server. All times are filled.
    Pong = 2,
    /// Phase 3, Server->client:
    /// Payload is relative to the client. All times are filled.
    Sync = 3,
};

/// Message exchanged between client and server to synchronize time.
/// Each interaction consists of three sequential messages aka phases.
struct ClockSynchronizerMessage
{
    unsigned magic_{};
    ClockSynchronizerPhase phase_{};
    unsigned localSent_{};
    unsigned remoteReceived_{};
    unsigned remoteSent_{};

    void Load(Deserializer& src);
    void Save(Serializer& dest) const;
};

class URHO3D_API ClockSynchronizer
{
public:
    /// Process incoming message. Should be called as soon as possible.
    virtual void ProcessMessage(const ClockSynchronizerMessage& msg) = 0;
    /// Receive outgoing message if there is any. Should be called as late as possible.
    virtual ea::optional<ClockSynchronizerMessage> PollMessage() = 0;

    /// Return whether synchronizer is ready to use.
    bool IsReady() const { return localToRemote_.IsInitialized(); }
    /// Convert from local to remote timestamp.
    unsigned LocalToRemote(unsigned value) const { return value + localToRemote_.GetFilteredValue(); }
    /// Convert from remote to local timestamp.
    unsigned RemoteToLocal(unsigned value) const { return value - localToRemote_.GetFilteredValue(); }
    /// Return ping, i.e. half of round-trip delay excluding remote processing time.
    unsigned GetPing() const { return roundTripDelay_.GetFilteredValue() / 2; }

protected:
    explicit ClockSynchronizer(unsigned clockBufferSize, unsigned pingBufferSize, ea::function<unsigned()> getTimestamp);
    unsigned GetTimestamp() const;
    void UpdateClocks(unsigned localSent, unsigned remoteReceived, unsigned remoteSent, unsigned localReceived);

private:
    const ea::function<unsigned()> getTimestamp_;
    FilteredUint localToRemote_;
    FilteredUint roundTripDelay_;
};

class URHO3D_API ServerClockSynchronizer : public ClockSynchronizer
{
public:
    ServerClockSynchronizer(unsigned pingIntervalMs, unsigned maxPingMs, unsigned clockBufferSize,
        unsigned pingBufferSize, ea::function<unsigned()> getTimestamp = nullptr);

    /// Implement ClockSynchronizer
    /// @{
    void ProcessMessage(const ClockSynchronizerMessage& msg) override;
    ea::optional<ClockSynchronizerMessage> PollMessage() override;
    /// @}

private:
    struct PendingPong
    {
        unsigned magic_{};
        unsigned serverSentTime_{};
    };
    struct PendingSync
    {
        unsigned magic_{};
        unsigned clientSentTime_{};
        unsigned serverReceivedTime_{};
    };

    ClockSynchronizerMessage CreateNewPing(unsigned now);
    ClockSynchronizerMessage CreateNewSync(const PendingSync& sync, unsigned now) const;
    void CleanupExpiredPings(unsigned now);

    const unsigned pingIntervalMs_{};
    const unsigned maxPingMs_{};

    ea::optional<unsigned> latestProbeTimestamp_;
    ea::vector<PendingPong> pendingPings_;
    ea::vector<PendingSync> pendingSyncs_;
};

class URHO3D_API ClientClockSynchronizer : public ClockSynchronizer
{
public:
    ClientClockSynchronizer(
        unsigned clockBufferSize, unsigned pingBufferSize, ea::function<unsigned()> getTimestamp = nullptr);

    /// Implement ClockSynchronizer
    /// @{
    void ProcessMessage(const ClockSynchronizerMessage& msg) override;
    ea::optional<ClockSynchronizerMessage> PollMessage() override;
    /// @}

private:
    struct PendingPong
    {
        unsigned magic_{};
        unsigned serverSentTime_{};
        unsigned clientReceivedTime_{};
    };

    ea::vector<PendingPong> pendingPongs_;
};

}
