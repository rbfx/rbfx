//
// Copyright (c) 2017-2021 the rbfx project.
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

#include <Urho3D/Container/ByteVector.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Network/AbstractConnection.h>
#include <Urho3D/Replica/ReplicationManager.h>

#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

using namespace Urho3D;

namespace Urho3D
{

class Node;
class PrefabResource;

}

namespace Tests
{

struct ConnectionQuality
{
    float minPing_{};
    float maxPing_{};
    float spikePing_{};
    float dropRate_{};
    float shuffleRate_{};
};

/// Test implementation of AbstractConnection with manual control over message transmission.
class ManualConnection : public AbstractConnection
{
public:
    static unsigned systemTime;

    ManualConnection(Context* context, ReplicationManager* sink, unsigned seed);

    void SetSinkConnection(AbstractConnection* sinkConnection) { sinkConnection_ = sinkConnection; }
    void SetQuality(const ConnectionQuality& quality) { quality_ = quality; }

    void SendMessageInternal(NetworkMessageId messageId, const unsigned char* data, unsigned numBytes, PacketTypeFlags packetType = PacketType::ReliableOrdered) override;
    ea::string ToString() const override { return "Manual Connection"; }
    bool IsClockSynchronized() const override { return true; }
    unsigned RemoteToLocalTime(unsigned time) const override { return time; }
    unsigned LocalToRemoteTime(unsigned time) const override { return time; }
    unsigned GetLocalTime() const override { return systemTime; }
    unsigned GetLocalTimeOfLatestRoundtrip() const override { return systemTime; }
    unsigned GetPing() const override { return RoundToInt(1000 * (quality_.minPing_ + quality_.maxPing_) / 2); }

    void IncrementTime(unsigned delta);

private:
    struct InternalMessage
    {
        unsigned receiveTime_{};
        NetworkMessageId messageId_{};
        ByteVector data_;
    };

    unsigned GetPing();
    void DeliverMessage(const InternalMessage& msg);
    void SendOrderedMessages(ea::vector<InternalMessage>& messages);
    void SendUnorderedMessages(ea::vector<InternalMessage>& messages);

    ReplicationManager* sink_{};
    AbstractConnection* sinkConnection_{};
    RandomEngine random_;

    ConnectionQuality quality_;

    unsigned currentTime_{};
    ea::vector<InternalMessage> messages_[2][2];

    unsigned totalMessages_{};
    unsigned totalUnorderedMessages_{};
    unsigned totalUnreliableMessages_{};
    unsigned droppedMessages_{};
    unsigned shuffledMessages_{};
};

/// Network simulator for tests.
/// There are 1024 "milliseconds" in a "second" to avoid dealing with floating point precision issues.
class NetworkSimulator
{
public:
    /// Number of frames per second.
    static constexpr unsigned FramesInSecond = 25;
    static constexpr unsigned MillisecondsInFrame = 1000 / FramesInSecond;
    static constexpr unsigned MillisecondsInQuant = 10;
    static double QuantizeDuration(double duration, unsigned millisecondsInQuant = MillisecondsInQuant);

    explicit NetworkSimulator(Scene* serverScene, unsigned seed = 0);
    ~NetworkSimulator();

    void AddClient(Scene* clientScene, const ConnectionQuality& quality);
    void RemoveClient(Scene* clientScene);

    static void SimulateEngineFrame(Context* context, float timeStep);
    static void SimulateTime(Context* context, float time, unsigned millisecondsInQuant = MillisecondsInQuant);

    void SimulateEngineFrame(float timeStep);
    void SimulateTime(float time, unsigned millisecondsInQuant = MillisecondsInQuant);

    AbstractConnection* GetServerToClientConnection(Scene* clientScene);

    RandomEngine& GetRandom() { return random_; }

private:
    static void SimulateTimeCallback(float time, unsigned millisecondsInQuant, ea::function<void(float timeStep)> callback);

    auto FindClientIter(Scene* scene)
    {
        const auto isSameScene = [&](const PerClient& data) { return data.clientScene_ == scene; };
        return ea::find_if(clients_.begin(), clients_.end(), isSameScene);
    }

    struct PerClient
    {
        Scene* clientScene_;
        ReplicationManager* clientReplicationManager_{};
        SharedPtr<Tests::ManualConnection> clientToServer_;
        SharedPtr<Tests::ManualConnection> serverToClient_;
    };

    Context* context_{};
    Network* network_{};
    RandomEngine random_;

    Scene* serverScene_{};
    ReplicationManager* serverReplicationManager_{};

    ea::vector<PerClient> clients_;
};

/// Spawn networked object on server.
Node* SpawnOnServer(Node* parent, StringHash objectType, PrefabResource* prefab, const ea::string& name,
    const Vector3& position = Vector3::ZERO, const Quaternion& rotation = Quaternion::IDENTITY);

template <class T>
Node* SpawnOnServer(Node* parent, PrefabResource* prefab, const ea::string& name, const Vector3& position = Vector3::ZERO,
    const Quaternion& rotation = Quaternion::IDENTITY)
{
    return SpawnOnServer(parent, T::GetTypeStatic(), prefab, name, position, rotation);
}

}
