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

#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/LocalClockSynchronizer.h"
#include "../Network/NetworkTime.h"
#include "../Network/ProtocolMessages.h"

#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>
#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

class AbstractConnection;
class Network;
class NetworkObject;
class NetworkManagerBase;
class Scene;

/// Client-only NetworkManager settings.
struct ClientSynchronizationSettings
{
    float clientTimeDelayInSeconds_{ 0.1f };
    float timeSnapThresholdInSeconds_{ 5.0 };
    float timeRewindThresholdInSeconds_{ 0.6 };

    float minTimeStepScale_{ 0.5f };
    float maxTimeStepScale_{ 2.0f };
};

struct ClientNetworkManagerSettings : public ClientSynchronizationSettings
{
    float traceDurationInSeconds_{ 1.0f };
    float pingSmoothConstant_{ 0.05f };
    float positionExtrapolationTimeInSeconds_{0.25};
};

/// Helper class that keeps clock synchronized with server.
class URHO3D_API ClientSynchronizationManager : public NonCopyable
{
public:
    ClientSynchronizationManager(
        Scene* scene, const MsgSynchronize& msg, const ClientSynchronizationSettings& settings);
    ~ClientSynchronizationManager();
    void SetSettings(const ClientSynchronizationSettings& settings) { settings_ = settings; }

    double MillisecondsToFrames(double valueMs) const;
    double SecondsToFrames(double valueSec) const;

    void ProcessClockUpdate(const MsgClock& msg);
    float ApplyTimeStep(float timeStep);

    /// Return current state
    /// @{
    bool IsNewFrame() const { return isNewFrame_; }
    unsigned GetConnectionId() const { return thisConnectionId_; }
    unsigned GetUpdateFrequency() const { return updateFrequency_; }
    unsigned GetLatestPingMs() const { return latestPing_; }
    NetworkTime GetServerTime() const { return serverTime_; }
    NetworkTime GetSmoothClientTime() const { return smoothClientTime_; }
    NetworkTime GetLatestUnstableClientTime() const { return latestUnstableClientTime_; }
    /// @}

private:
    void ProcessPendingClockUpdate(const MsgClock& msg);
    void ResetServerAndClientTime(const NetworkTime& serverTime);
    float UpdateSmoothClientTime(float timeStep);

    void CheckAndAdjustTime(
        NetworkTime& time, ea::ring_buffer<double>& errors, double error, unsigned numTrimmedSamples, bool quiet) const;
    ea::optional<double> UpdateAverageError(ea::ring_buffer<double>& errors, double error, unsigned numTrimmedSamples) const;
    void AdjustTime(NetworkTime& time, ea::ring_buffer<double>& errors, double adjustment) const;

    NetworkTime ToServerTime(unsigned lastServerFrame, unsigned pingMs) const;
    NetworkTime ToClientTime(const NetworkTime& serverTime) const;

    Scene* scene_{};

    const unsigned thisConnectionId_{};
    const unsigned updateFrequency_{};
    const unsigned numSamples_{};
    const unsigned numTrimmedSamples_{};

    ClientSynchronizationSettings settings_{};

    NetworkTime latestServerTime_{};
    unsigned latestPing_{};

    NetworkTime serverTime_;
    NetworkTime clientTime_;
    NetworkTime smoothClientTime_;
    NetworkTime latestUnstableClientTime_{};
    bool isNewFrame_{};

    ea::vector<MsgClock> pendingClockUpdates_;
    ea::ring_buffer<double> serverTimeErrors_{};
    ea::ring_buffer<double> clientTimeErrors_{};

    PhysicsClockSynchronizer physicsSync_;
};

/// Client part of NetworkManager subsystem.
class URHO3D_API ClientNetworkManager : public Object
{
    URHO3D_OBJECT(ClientNetworkManager, Object);

public:
    ClientNetworkManager(NetworkManagerBase* base, Scene* scene, AbstractConnection* connection);

    void ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData);

    ea::string GetDebugInfo() const;
    AbstractConnection* GetConnection() const { return connection_; }
    const ClientNetworkManagerSettings& GetSettings() const { return settings_; }

    /// Return global properties of client state.
    /// @{
    unsigned GetPingMs() const { return sync_ ? sync_->GetLatestPingMs() : 0; }
    bool IsSynchronized() const { return sync_.has_value(); }
    NetworkTime GetServerTime() const { return sync_ ? sync_->GetServerTime() : NetworkTime{}; }
    NetworkTime GetClientTime() const { return sync_ ? sync_->GetSmoothClientTime() : NetworkTime{}; }

    unsigned GetCurrentFrame() const { return GetServerTime().GetFrame(); }
    float GetCurrentBlendFactor() const { return GetServerTime().GetSubFrame(); }
    unsigned GetLastSynchronizationFrame() const { return sync_ ? sync_->GetLatestUnstableClientTime().GetFrame() : 0; }
    double GetCurrentFrameDeltaRelativeTo(unsigned referenceFrame) const;

    unsigned GetTraceCapacity() const;
    unsigned GetPositionExtrapolationFrames() const;
    /// @}

private:
    void UpdateReplica(float timeStep);
    void SendObjectsFeedbackUnreliable(unsigned feedbackFrame);

    NetworkObject* CreateNetworkObject(NetworkId networkId, StringHash componentType, bool isOwned);
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

    ea::optional<ClientSynchronizationManager> sync_;
    ea::unordered_set<WeakPtr<NetworkObject>> ownedObjects_;

    VectorBuffer componentBuffer_;
};

}
