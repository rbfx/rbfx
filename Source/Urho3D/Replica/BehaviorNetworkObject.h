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

#include "../Replica/StaticNetworkObject.h"

namespace Urho3D
{

class BehaviorNetworkObject;

/// Aspect of network behavior that is injected into BehaviorNetworkObject.
///
/// NetworkBehavior should be created only after owner BehaviorNetworkObject is created,
/// but before it's replicated to clients (on server) or creation is finished (on client).
/// This basically means that list of NetworkBehavior-s attached to BehaviorNetworkObject
/// should stay the same during all lifetime of BehaviorNetworkObject.
class URHO3D_API NetworkBehavior
    : public Component
    , public NetworkCallback
{
    URHO3D_OBJECT(NetworkBehavior, Component);

public:
    NetworkBehavior(Context* context, NetworkCallbackFlags callbackMask);
    ~NetworkBehavior() override;

    static void RegisterObject(Context* context);

    /// Internal. Set owner NetworkObject.
    void SetNetworkObject(BehaviorNetworkObject* owner);
    /// Internal. Find closest parent NetworkObject.
    NetworkObject* FindClosestNetworkObject() const;

    /// Return owner NetworkObject.
    BehaviorNetworkObject* GetNetworkObject() const { return owner_; }
    /// Return callback mask.
    NetworkCallbackFlags GetCallbackMask() const { return callbackMask_; }

protected:
    /// Component implementation
    /// @{
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// @}

private:
    WeakPtr<BehaviorNetworkObject> owner_;
    NetworkCallbackFlags callbackMask_;
};

/// NetworkObject that is composed from the fixed amount of independent behaviors.
/// Both client and server are responsible for creating same ,
/// e.g. from prefabs.
class URHO3D_API BehaviorNetworkObject : public StaticNetworkObject
{
    URHO3D_OBJECT(BehaviorNetworkObject, StaticNetworkObject);

public:
    const unsigned MaxNumBehaviors = 29; // Current implementation of VLE supports only 29 bits

    explicit BehaviorNetworkObject(Context* context);
    ~BehaviorNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Internal. Mark NetworkObject as invalid and disable all behaviors.
    void InvalidateBehaviors();
    /// Return behavior by type.
    NetworkBehavior* GetNetworkBehavior(StringHash componentType, unsigned index = 0) const;
    template <class T> T* GetNetworkBehavior(unsigned index = 0) const { return static_cast<T*>(GetNetworkBehavior(T::GetTypeStatic(), index)); }

    /// Implement NetworkObject.
    /// @{
    void InitializeStandalone() override;
    void InitializeOnServer() override;
    void WriteSnapshot(NetworkFrame frame, Serializer& dest) override;
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

    ea::optional<NetworkObjectRelevance> GetRelevanceForClient(AbstractConnection* connection) override;
    void UpdateTransformOnServer() override;
    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) override;

    bool PrepareReliableDelta(NetworkFrame frame) override;
    void WriteReliableDelta(NetworkFrame frame, Serializer& dest) override;
    void ReadReliableDelta(NetworkFrame frame, Deserializer& src) override;

    bool PrepareUnreliableDelta(NetworkFrame frame) override;
    void WriteUnreliableDelta(NetworkFrame frame, Serializer& dest) override;
    void ReadUnreliableDelta(NetworkFrame frame, Deserializer& src) override;

    bool PrepareUnreliableFeedback(NetworkFrame frame) override;
    void WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest) override;
    void ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src) override;

    void Update(float replicaTimeStep, float inputTimeStep) override;
    /// @}

private:
    void InitializeBehaviors();

    struct ConnectedNetworkBehavior
    {
        unsigned bit_{};
        WeakPtr<NetworkBehavior> component_;
        NetworkCallbackFlags callbackMask_;
    };

    ea::vector<ConnectedNetworkBehavior> behaviors_;
    NetworkCallbackFlags callbackMask_;

    unsigned reliableUpdateMask_{};
    unsigned unreliableUpdateMask_{};
    unsigned unreliableFeedbackMask_{};
};

};
