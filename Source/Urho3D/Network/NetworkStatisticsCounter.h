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
//
// NetworkStatisticsCounter is a helper that tracks packet and byte rates.
//

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/Network/PacketTypeFlags.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"

namespace Urho3D
{

struct NetworkStatisticsSnapshot
{
    uint64_t bytesInPerSec_{};
    uint64_t bytesOutPerSec_{};
    int packetsInPerSec_{};
    int packetsOutPerSec_{};

    NetworkStatisticsSnapshot& operator+=(const NetworkStatisticsSnapshot& rhs)
    {
        bytesInPerSec_ += rhs.bytesInPerSec_;
        bytesOutPerSec_ += rhs.bytesOutPerSec_;
        packetsInPerSec_ += rhs.packetsInPerSec_;
        packetsOutPerSec_ += rhs.packetsOutPerSec_;
        return *this;
    }
};

class NetworkStatisticsCounter
{
public:
    NetworkStatisticsCounter(unsigned windowSize = 10, unsigned intervalMs = 1000)
        : packetCounterIncoming_(windowSize, intervalMs)
        , packetCounterOutgoing_(windowSize, intervalMs)
        , bytesCounterIncoming_(windowSize, intervalMs)
        , bytesCounterOutgoing_(windowSize, intervalMs)
    {
    }

    NetworkStatisticsCounter(NetworkConnection* connection, unsigned windowSize = 10, unsigned intervalMs = 1000)
        : NetworkStatisticsCounter(windowSize, intervalMs)
    {
        Attach(connection);
    }

    /// Subscribe to connection signals for automatic packet statistics.
    void Attach(NetworkConnection* connection)
    {
        if (!connection || connection_.Get() == connection)
            return;

        connection_ = connection;

        connection->onSendData_.SubscribeWithSender(connection,
            [this](NetworkConnection* sender, const MemoryBuffer& data, PacketTypeFlags type, bool& handled)
        {
            (void)sender;
            (void)type;
            if (!handled)
                AddOutgoingPacket(data.GetSize());
        });

        connection->onData_.SubscribeWithSender(connection,
            [this](NetworkConnection* sender, MemoryBuffer& message, bool& handled)
        {
            (void)sender;
            (void)handled;
            AddIncomingPacket(message.GetSize());
        });
    }

    void AddIncomingPacket(unsigned bytes)
    {
        packetCounterIncoming_.AddSample(1);
        bytesCounterIncoming_.AddSample(bytes);
    }

    void AddOutgoingPacket(unsigned bytes)
    {
        packetCounterOutgoing_.AddSample(1);
        bytesCounterOutgoing_.AddSample(bytes);
    }

    uint64_t GetBytesInPerSec()
    {
        return static_cast<uint64_t>(bytesCounterIncoming_.GetLast());
    }

    uint64_t GetBytesOutPerSec()
    {
        return static_cast<uint64_t>(bytesCounterOutgoing_.GetLast());
    }

    int GetPacketsInPerSec() { return static_cast<int>(packetCounterIncoming_.GetLast()); }
    int GetPacketsOutPerSec() { return static_cast<int>(packetCounterOutgoing_.GetLast()); }

    NetworkStatisticsSnapshot GetSnapshot()
    {
        NetworkStatisticsSnapshot snapshot;
        snapshot.bytesInPerSec_ = GetBytesInPerSec();
        snapshot.bytesOutPerSec_ = GetBytesOutPerSec();
        snapshot.packetsInPerSec_ = GetPacketsInPerSec();
        snapshot.packetsOutPerSec_ = GetPacketsOutPerSec();
        return snapshot;
    }

private:
    TimedCounter packetCounterIncoming_;
    TimedCounter packetCounterOutgoing_;
    TimedCounter bytesCounterIncoming_;
    TimedCounter bytesCounterOutgoing_;
    WeakPtr<NetworkConnection> connection_{};
};

} // namespace Urho3D
