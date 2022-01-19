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
struct NetworkSetting;

/// Client-only NetworkManager settings.
struct ClientSynchronizationSettings
{
    float clientTimeDelayInSeconds_{ 0.1f };
    float timeSnapThresholdInSeconds_{ 5.0 };
    float timeRewindThresholdInSeconds_{ 0.6 };

    float minTimeStepScale_{ 0.5f };
    float maxTimeStepScale_{ 2.0f };
    float positionSmoothConstant_{ 15.0f };
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
    ClientSynchronizationManager(Scene* scene, AbstractConnection* connection, const MsgSceneClock& msg,
        const VariantMap& serverSettings, const ClientSynchronizationSettings& settings);
    ~ClientSynchronizationManager();
    void SetSettings(const ClientSynchronizationSettings& settings) { settings_ = settings; }

    double MillisecondsToFrames(double valueMs) const;
    double SecondsToFrames(double valueSec) const;

    float ApplyTimeStep(float timeStep, const ea::vector<MsgSceneClock>& pendingClockUpdates);

    /// Return current state
    /// @{
    bool IsNewInputFrame() const { return synchronizedPhysicsTick_.has_value(); }
    ea::optional<unsigned> GetSynchronizedPhysicsTick() const { return synchronizedPhysicsTick_; }
    unsigned GetConnectionId() const { return thisConnectionId_; }
    unsigned GetUpdateFrequency() const { return updateFrequency_; }
    unsigned GetInputDelay() const { return inputDelay_; }
    NetworkTime GetServerTime() const { return serverTime_; }
    NetworkTime GetReplicaTime() const { return replicaTime_.Get(); }
    NetworkTime GetInputTime() const { return inputTime_.Get(); }
    NetworkTime GetLatestScaledInputTime() const { return latestScaledInputTime_; }
    const Variant& GetSetting(const NetworkSetting& setting) const;
    /// @}

private:
    void UpdateServerTime(const MsgSceneClock& msg, bool skipOutdated);
    void ProcessPendingClockUpdate(const MsgSceneClock& msg);
    void ResetServerAndClientTime(const NetworkTime& serverTime);
    float UpdateSmoothClientTime(float timeStep);

    void CheckAndAdjustTime(
        NetworkTime& time, ea::ring_buffer<double>& errors, double error, unsigned numTrimmedSamples, bool quiet) const;
    ea::optional<double> UpdateAverageError(ea::ring_buffer<double>& errors, double error, unsigned numTrimmedSamples) const;
    void AdjustTime(NetworkTime& time, ea::ring_buffer<double>& errors, double adjustment) const;

    NetworkTime ToClientTime(const NetworkTime& serverTime) const;
    NetworkTime ToInputTime(const NetworkTime& serverTime) const;

    Scene* scene_{};
    AbstractConnection* connection_{};
    const VariantMap serverSettings_;

    const unsigned thisConnectionId_{};
    const unsigned updateFrequency_{};
    const float timeSnapThreshold_{};
    const float timeErrorTolerance_{};
    const float minTimeDilation_{};
    const float maxTimeDilation_{};

    ClientSynchronizationSettings settings_{};

    unsigned inputDelay_{};
    NetworkTime serverTime_;
    unsigned latestServerFrame_{};
    NetworkTime latestScaledInputTime_{};
    ea::optional<unsigned> synchronizedPhysicsTick_{};

    SoftNetworkTime replicaTime_;
    SoftNetworkTime inputTime_;
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
    const ClientNetworkManagerSettings& GetSettings() const { return settings2_; }

    /// Return global properties of client state.
    /// @{
    bool IsSynchronized() const { return sync_.has_value(); }
    NetworkTime GetServerTime() const { return sync_ ? sync_->GetServerTime() : NetworkTime{}; }
    NetworkTime GetClientTime() const { return sync_ ? sync_->GetReplicaTime() : NetworkTime{}; }
    NetworkTime GetInputTime() const { return sync_ ? sync_->GetInputTime() : NetworkTime{}; }

    unsigned GetCurrentFrame() const { return GetServerTime().GetFrame(); }
    float GetCurrentBlendFactor() const { return GetServerTime().GetSubFrame(); }
    unsigned GetLatestScaledInputFrame() const { return sync_ ? sync_->GetLatestScaledInputTime().GetFrame() : 0; }
    double GetCurrentFrameDeltaRelativeTo(unsigned referenceFrame) const;

    unsigned GetTraceCapacity() const;
    unsigned GetPositionExtrapolationFrames() const;
    /// @}

private:
    void SynchronizeClocks(float timeStep);
    void UpdateReplica(float timeStep);
    void SendObjectsFeedbackUnreliable(unsigned feedbackFrame);

    NetworkObject* CreateNetworkObject(NetworkId networkId, StringHash componentType, bool isOwned);
    NetworkObject* GetCheckedNetworkObject(NetworkId networkId, StringHash componentType);
    void RemoveNetworkObject(WeakPtr<NetworkObject> networkObject);

    void ProcessConfigure(const MsgConfigure& msg);
    void ProcessSceneClock(const MsgSceneClock& msg);
    void ProcessRemoveObjects(MemoryBuffer& messageData);
    void ProcessAddObjects(MemoryBuffer& messageData);
    void ProcessUpdateObjectsReliable(MemoryBuffer& messageData);
    void ProcessUpdateObjectsUnreliable(MemoryBuffer& messageData);

    ea::optional<VariantMap> serverSettings_;
    ea::optional<unsigned> synchronizationMagic_;
    ClientNetworkManagerSettings settings2_;

    Network* network_{};
    NetworkManagerBase* base_{};
    Scene* scene_{};
    AbstractConnection* connection_{};

    ea::vector<MsgSceneClock> pendingClockUpdates_;
    ea::optional<ClientSynchronizationManager> sync_;
    ea::unordered_set<WeakPtr<NetworkObject>> ownedObjects_;

    VectorBuffer componentBuffer_;
};

}
