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

#include "../Container/IndexAllocator.h"
#include "../Core/Timer.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/ProtocolMessages.h"

#include <EASTL/bitvector.h>
#include <EASTL/optional.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

class AbstractConnection;
class Network;
class NetworkObject;
class NetworkManagerBase;
class Scene;

struct ClientPing
{
    unsigned magic_{};
    HiresTimer timer_;
};

/// Per-connection data for server.
struct ClientConnectionData
{
    AbstractConnection* connection_{};

    ea::ring_buffer<unsigned> comfirmedPings_{};
    ea::vector<unsigned> comfirmedPingsSorted_;
    ea::ring_buffer<ClientPing> pendingPings_{};
    ea::optional<unsigned> overridePing_;
    unsigned averagePing_{ M_MAX_UNSIGNED };

    bool synchronized_{};
    ea::optional<unsigned> pendingSynchronization_{};

    float pingAccumulator_{};
    float clockAccumulator_{};

    ea::bitvector<> isComponentReplicated_;
    ea::vector<float> componentsRelevanceTimeouts_;

    ea::vector<NetworkId> pendingRemovedComponents_;
    ea::vector<ea::pair<NetworkObject*, bool>> pendingUpdatedComponents_;
};

/// Server settings for NetworkManager.
struct ServerNetworkManagerSettings
{
    unsigned numInitialPings_{ 11 };
    unsigned numTrimmedMaxPings_{ 3 };
    unsigned pingIntervalMs_{ 1000 };
    unsigned maxOngoingPings_{ 11 };

    unsigned clockIntervalMs_{ 250 };
    unsigned numStartClockSamples_{ 17 };
    unsigned numOngoingClockSamples_{ 41 };
    unsigned numTrimmedClockSamples_{ 3 };

    float relevanceTimeout_{ 5.0f };
};

/// Server part of NetworkManager subsystem.
class URHO3D_API ServerNetworkManager : public Object
{
    URHO3D_OBJECT(ServerNetworkManager, Object);

public:
    ServerNetworkManager(NetworkManagerBase* base, Scene* scene);

    void AddConnection(AbstractConnection* connection);
    void RemoveConnection(AbstractConnection* connection);
    void SendUpdate(AbstractConnection* connection);
    void ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData);

    void SetTestPing(AbstractConnection* connection, unsigned ping);
    void SetCurrentFrame(unsigned frame);

    ea::string ToString() const;
    unsigned GetCurrentFrame() const { return currentFrame_; }

private:
    void BeginNetworkFrame();
    ClientConnectionData& GetConnection(AbstractConnection* connection);

    void RecalculateAvergagePing(ClientConnectionData& data);
    unsigned GetMagic(bool reliable = false) const;

    Network* network_{};
    NetworkManagerBase* base_{};
    Scene* scene_{};
    const ServerNetworkManagerSettings settings_; // TODO: Make mutable

    unsigned updateFrequency_{};
    unsigned currentFrame_{};

    ea::unordered_map<AbstractConnection*, ClientConnectionData> connections_;
    VectorBuffer componentBuffer_;
    ea::vector<NetworkObject*> orderedNetworkObjects_;
};

}
