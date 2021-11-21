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
#include <Urho3D/Network/NetworkManager.h>

#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

using namespace Urho3D;

namespace Tests
{

/// Test implementation of AbstractConnection with manual control over message transmission.
class ManualConnection : public AbstractConnection
{
public:
    ManualConnection(Context* context, NetworkManager* sink);

    void SetSinkConnection(AbstractConnection* sinkConnection) { sinkConnection_ = sinkConnection; }
    void SetPing(unsigned minPing, unsigned maxPing) { minPing_ = minPing; maxPing_ = maxPing; }

    void SendMessageInternal(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes) override;
    ea::string ToString() const override { return "Manual Connection"; }

    void IncrementTime(unsigned delta);

private:
    struct InternalMessage
    {
        unsigned sendingTime_{};
        NetworkMessageId messageId_{};
        ByteVector data_;
    };

    NetworkManager* sink_{};
    AbstractConnection* sinkConnection_{};
    RandomEngine random_{ 0 };

    unsigned minPing_{};
    unsigned maxPing_{};

    unsigned currentTime_{};
    ea::vector<InternalMessage> messages_[2][2];
};

/// Network simulator for tests.
/// There are 1024 "milliseconds" in a "second" to avoid dealing with floating point precision issues.
class NetworkSimulator
{
public:
    /// Number of frames per second.
    static const unsigned FramesInSecond = 32;
    /// Number of "milliseconds" in one server frame. Use decently big value to have sub-frame control over execution.
    static const unsigned MillisecondsInFrame = 32;

    explicit NetworkSimulator(Scene* serverScene);
    void AddClient(Scene* clientScene, float minPing, float maxPing);

    void SimulateEngineFrame(float timeStep);
    void SimulateTime(float time);

private:
    struct PerClient
    {
        Scene* clientScene_;
        NetworkManager* clientNetworkManager_{};
        SharedPtr<Tests::ManualConnection> clientToServer_;
        SharedPtr<Tests::ManualConnection> serverToClient_;
    };

    Context* context_{};
    Network* network_{};

    Scene* serverScene_{};
    NetworkManager* serverNetworkManager_{};

    ea::vector<PerClient> clientScenes_;
};

SharedPtr<Context> CreateNetworkSimulatorContext();

}
