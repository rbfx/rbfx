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

/// \file

#pragma once

#include "../Container/IndexAllocator.h"
#include "../Core/Timer.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/ClockSynchronizer.h"
#include "../Replica/ClientInputStatistics.h"
#include "../Replica/NetworkId.h"
#include "../Replica/TickSynchronizer.h"
#include "../Replica/ProtocolMessages.h"

#include <EASTL/bitvector.h>
#include <EASTL/optional.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

class AbstractConnection;
class Network;
class NetworkObject;
class NetworkObjectRegistry;
class Scene;
struct NetworkSetting;

/// Replication state shared between all clients.
class SharedReplicationState : public RefCounted
{
public:
    explicit SharedReplicationState(NetworkObjectRegistry* objectRegistry);

    /// Initial preparation for network update.
    void PrepareForUpdate();
    /// Request delta update to be prepared for specified object.
    void QueueDeltaUpdate(NetworkObject* networkObject);
    /// Cook all requested delta updates.
    void CookDeltaUpdates(NetworkFrame currentFrame);

    /// Return state of the current frame.
    /// @{
    const ea::unordered_set<NetworkId>& GetRecentlyRemovedObjects() const { return recentlyRemovedObjects_; }
    const ea::vector<NetworkObject*>& GetSortedObjects() const { return sortedNetworkObjects_; }
    unsigned GetIndexUpperBound() const;
    const ea::unordered_set<NetworkObject*>& GetOwnedObjectsByConnection(AbstractConnection* connection) const;
    ea::optional<ConstByteSpan> GetReliableUpdateByIndex(unsigned index) const;
    ea::optional<ConstByteSpan> GetUnreliableUpdateByIndex(unsigned index) const;
    /// @}

private:
    /// A span in delta update buffer corresponding to the update data of the individual NetworkObject.
    struct DeltaBufferSpan
    {
        unsigned beginOffset_{};
        unsigned endOffset_{};
    };

    void OnNetworkObjectAdded(NetworkObject* networkObject);
    void OnNetworkObjectRemoved(NetworkObject* networkObject);

    void ResetFrameBuffers();
    void InitializeNewObjects();

    ConstByteSpan GetSpanData(const DeltaBufferSpan& span) const;

    const WeakPtr<NetworkObjectRegistry> objectRegistry_{};

    ea::unordered_set<NetworkId> recentlyRemovedObjects_;
    ea::unordered_set<NetworkId> recentlyAddedObjects_;

    ea::vector<NetworkObject*> sortedNetworkObjects_;

    ea::vector<bool> isDeltaUpdateQueued_;
    ea::vector<bool> needReliableDeltaUpdate_;
    ea::vector<bool> needUnreliableDeltaUpdate_;

    VectorBuffer deltaUpdateBuffer_;
    ea::vector<DeltaBufferSpan> reliableDeltaUpdateData_;
    ea::vector<DeltaBufferSpan> unreliableDeltaUpdateData_;

    ea::unordered_map<AbstractConnection*, ea::unordered_set<NetworkObject*>> ownedObjectsByConnection_;
};

/// Clock synchronization state specific to individual client connection.
class ClientSynchronizationState : public RefCounted
{
public:
    ClientSynchronizationState(
        NetworkObjectRegistry* objectRegistry, AbstractConnection* connection, const VariantMap& settings);

    /// Begin network frame. Overtime indicates how much time has passed since actual frame start time.
    void BeginNetworkFrame(NetworkFrame currentFrame, float overtime);

    /// Return current state and properties
    /// @{
    const Variant& GetSetting(const NetworkSetting& setting) const;
    bool IsSynchronized() const { return synchronized_; }
    NetworkFrame GetCurrentFrame() const { return frame_; }
    unsigned GetInputDelay() const { return inputDelay_; };
    unsigned GetInputBufferSize() const { return inputBufferSize_; };
    /// @}

protected:
    /// Send messages to connection for current frame.
    void SendMessages();
    /// Process messages for this client.
    bool ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData);
    /// Notify statistics aggregator that user input has received for specified frame.
    void OnInputReceived(NetworkFrame inputFrame);

    const WeakPtr<NetworkObjectRegistry> objectRegistry_;
    const WeakPtr<AbstractConnection> connection_;
    VariantMap settings_;
    const unsigned updateFrequency_{};

private:
    void UpdateInputDelay();
    void UpdateInputBuffer();
    void ProcessSynchronized(const MsgSynchronized& msg);
    unsigned MakeMagic() const;

    static constexpr unsigned InputStatsSafetyLimit = 64;

    ea::optional<unsigned> synchronizationMagic_;
    bool synchronized_{};

    NetworkFrame frame_{};
    unsigned frameLocalTime_{};

    ea::optional<unsigned> latestProcessedPingTimestamp_;
    FilteredUint inputDelayFilter_;
    unsigned inputDelay_{};

    ClientInputStatistics inputStats_;
    FilteredUint inputBufferFilter_;
    unsigned inputBufferSize_{};

    float clockTimeAccumulator_{};
};

/// Scene replication state specific to individual client connection.
struct ClientReplicationState : public ClientSynchronizationState
{
public:
    ClientReplicationState(
        NetworkObjectRegistry* objectRegistry, AbstractConnection* connection, const VariantMap& settings);

    /// Perform network update from the perspective of this client connection.
    void UpdateNetworkObjects(SharedReplicationState& sharedState);

    /// Process messages for this client.
    bool ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData);
    /// Send messages to connection for current frame.
    void SendMessages(NetworkFrame currentFrame, const SharedReplicationState& sharedState);

    /// Manage reported input loss.
    /// @{
    void SetReportedInputLoss(float loss) { reportedLoss_ = loss; }
    float GetReportedInputLoss() const { return reportedLoss_;}
    /// @}

private:
    void ProcessObjectsFeedbackUnreliable(MemoryBuffer& messageData);
    void SendRemoveObjects();
    void SendAddObjects();
    void SendUpdateObjectsReliable(const SharedReplicationState& sharedState);
    void SendUpdateObjectsUnreliable(NetworkFrame currentFrame, const SharedReplicationState& sharedState);

    ea::vector<NetworkObjectRelevance> objectsRelevance_;
    ea::vector<float> objectsRelevanceTimeouts_;

    ea::vector<NetworkId> pendingRemovedObjects_;
    ea::vector<ea::pair<NetworkObject*, bool>> pendingUpdatedObjects_;

    VectorBuffer componentBuffer_;

    float reportedLoss_{};
};

/// Server part of ReplicationManager subsystem.
class URHO3D_API ServerReplicator : public Object
{
    URHO3D_OBJECT(ServerReplicator, Object);

public:
    explicit ServerReplicator(Scene* scene);
    ~ServerReplicator() override;

    void AddConnection(AbstractConnection* connection);
    void RemoveConnection(AbstractConnection* connection);
    bool ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData);
    void ProcessSceneUpdate();
    void ReportInputLoss(AbstractConnection* connection, float percentLoss);

    void SetCurrentFrame(NetworkFrame frame);

    /// Return current state of the replicator.
    /// @{
    ea::string GetDebugInfo() const;
    const Variant& GetSetting(const NetworkSetting& setting) const;
    unsigned GetFeedbackDelay(AbstractConnection* connection) const;
    const ea::unordered_set<NetworkObject*>& GetNetworkObjectsOwnedByConnection(AbstractConnection* connection) const;
    NetworkObject* GetNetworkObjectOwnedByConnection(AbstractConnection* connection) const;
    NetworkTime GetServerTime() const { return NetworkTime{currentFrame_}; }
    unsigned GetUpdateFrequency() const { return updateFrequency_; }
    NetworkFrame GetCurrentFrame() const { return currentFrame_; }
    /// @}

private:
    void OnInputReady(float timeStep, bool isUpdateNow, float overtime);
    void OnNetworkUpdate();

    ClientReplicationState* GetClientState(AbstractConnection* connection) const;

    const WeakPtr<Network> network_;
    const WeakPtr<Scene> scene_;
    const WeakPtr<NetworkObjectRegistry> objectRegistry_;

    VariantMap settings_;

    const unsigned updateFrequency_{};
    NetworkFrame currentFrame_{};

    PhysicsTickSynchronizer physicsSync_;

    SharedPtr<SharedReplicationState> sharedState_;
    ea::unordered_map<AbstractConnection*, SharedPtr<ClientReplicationState>> connections_;
};

}
