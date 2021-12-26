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
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/NetworkTime.h"
#include "../Network/ProtocolMessages.h"

#include <EASTL/optional.h>
#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

class AbstractConnection;
class Network;
class NetworkObject;
class NetworkManagerBase;
class Scene;

/// Client-only NetworkManager settings.
struct ClientNetworkManagerSettings
{
    float traceDurationInSeconds_{ 1.0f };
    float sampleDelayInSeconds_{ 0.1f };
    float pingSmoothConstant_{ 0.05f };

    double clockRewindThresholdFrames_{ 0.6 };
    double clockSnapThresholdSec_{ 10.0 };
};

/// Client clock synchronized with server.
struct ClientClock
{
    ClientClock(unsigned updateFrequency, unsigned numStartSamples, unsigned numTrimmedSamples, unsigned numOngoingSamples);

    const unsigned updateFrequency_{};
    const unsigned numStartSamples_{};
    const unsigned numTrimmedSamples_{};
    const unsigned numOngoingSamples_{};

    unsigned latestServerFrame_{};
    unsigned ping_{};
    /// TODO: Stabilize ping before smoothing
    double smoothPing_{};

    NetworkTime serverTime_;
    float frameDuration_{};
    unsigned lastSynchronizationFrame_{};

    ea::ring_buffer<double> synchronizationErrors_{};
    ea::vector<double> synchronizationErrorsSorted_{};
    double averageError_{};
};

/// Client part of NetworkManager subsystem.
class URHO3D_API ClientNetworkManager : public Object
{
    URHO3D_OBJECT(ClientNetworkManager, Object);

public:
    ClientNetworkManager(NetworkManagerBase* base, Scene* scene, AbstractConnection* connection);

    void ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData);

    ea::string ToString() const;
    AbstractConnection* GetConnection() const { return connection_; }

    /// Return global properties of client state.
    /// @{
    unsigned GetPingInMs() const { return clock_ ? clock_->ping_ : 0; }
    bool IsSynchronized() const { return clock_.has_value(); }
    NetworkTime GetServerTime() const { return clock_ ? clock_->serverTime_ : NetworkTime{}; }

    unsigned GetCurrentFrame() const { return GetServerTime().GetFrame(); }
    float GetCurrentBlendFactor() const { return GetServerTime().GetSubFrame(); }
    unsigned GetLastSynchronizationFrame() const { return clock_ ? clock_->lastSynchronizationFrame_ : 0; }
    double GetCurrentFrameDeltaRelativeTo(unsigned referenceFrame) const;
    unsigned GetTraceCapacity() const;
    /// @}

private:
    void OnInputProcessed();
    void ProcessTimeCorrection();

    NetworkObject* CreateNetworkObject(NetworkId networkId, StringHash componentType);
    NetworkObject* GetCheckedNetworkObject(NetworkId networkId, StringHash componentType);
    void RemoveNetworkObject(WeakPtr<NetworkObject> networkObject);

    void ProcessPing(const MsgPingPong& msg);
    void ProcessSynchronize(const MsgSynchronize& msg);
    void ProcessClock(const MsgClock& msg);
    void ProcessRemoveObjects(MemoryBuffer& messageData);
    void ProcessAddObjects(MemoryBuffer& messageData);
    void ProcessUpdateObjectsReliable(MemoryBuffer& messageData);
    void ProcessUpdateObjectsUnreliable(MemoryBuffer& messageData);

    ClientNetworkManagerSettings settings_;

    Network* network_{};
    NetworkManagerBase* base_{};
    Scene* scene_{};
    AbstractConnection* connection_{};

    ea::optional<ClientClock> clock_;

    VectorBuffer componentBuffer_;
};

}
