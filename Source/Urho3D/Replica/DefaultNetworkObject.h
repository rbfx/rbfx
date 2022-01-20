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

#include "../Replica/NetworkObject.h"
#include "../Replica/NetworkValue.h"

namespace Urho3D
{

class BehaviorNetworkObject;
class XMLFile;

/// Default implementation of NetworkObject that does some basic replication.
class URHO3D_API DefaultNetworkObject : public NetworkObject
{
    URHO3D_OBJECT(DefaultNetworkObject, NetworkObject);

public:
    static constexpr unsigned ParentNetworkObjectIdMask = 1 << 0;
    static constexpr unsigned WorldTransformMask = 1 << 1;

    explicit DefaultNetworkObject(Context* context);
    ~DefaultNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Attribute modification. Don't do that after replication!
    /// @{
    void SetClientPrefab(XMLFile* prefab);
    /// @}

    /// Implementation of NetworkObject
    /// @{
    void InitializeOnServer() override;

    void UpdateTransformOnServer() override;
    void WriteSnapshot(unsigned frame, Serializer& dest) override;
    unsigned GetReliableDeltaMask(unsigned frame) override;
    void WriteReliableDelta(unsigned frame, unsigned mask, Serializer& dest) override;
    unsigned GetUnreliableDeltaMask(unsigned frame) override;
    void WriteUnreliableDelta(unsigned frame, unsigned mask, Serializer& dest) override;

    void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame) override;
    void ReadSnapshot(unsigned frame, Deserializer& src) override;
    void ReadReliableDelta(unsigned frame, Deserializer& src) override;
    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override;
    /// @}

    /// Getters for network properties
    /// @{
    Vector3 GetTemporalWorldPosition(const NetworkTime& time) const { return worldPositionTrace_.SampleValid(time); }
    Quaternion GetTemporalWorldRotation(const NetworkTime& time) const { return worldRotationTrace_.SampleValid(time); }
    ea::optional<Vector3> GetRawTemporalWorldPosition(unsigned frame) const { return worldPositionTrace_.GetRaw(frame); }
    ea::optional<Quaternion> GetRawTemporalWorldRotation(unsigned frame) const { return worldRotationTrace_.GetRaw(frame); }
    /// @}

protected:
    /// Read and write actual deltas and delta masks.
    /// @{
    virtual unsigned WriteReliableDeltaMask();
    virtual void WriteReliableDeltaPayload(unsigned mask, unsigned frame, Serializer& dest);
    virtual void ReadReliableDeltaPayload(unsigned mask, unsigned frame, Deserializer& src);

    virtual unsigned WriteUnreliableDeltaMask();
    virtual void WriteUnreliableDeltaPayload(unsigned mask, unsigned frame, Serializer& dest);
    virtual void ReadUnreliableDeltaPayload(unsigned mask, unsigned frame, Deserializer& src);
    /// @}

    ResourceRef GetClientPrefabAttr() const;
    void SetClientPrefabAttr(const ResourceRef& value);

private:
    static const unsigned WorldTransformCooldown = 8;

    /// Attributes
    /// @{
    SharedPtr<XMLFile> clientPrefab_;
    /// @}

    /// Delta update caches (for server)
    /// @{
    NetworkId lastParentNetworkId_{ InvalidNetworkId };
    unsigned worldTransformCounter_{};
    /// @}

    /// Synchronized values (for both client and server)
    /// @{
    NetworkValue<Vector3> worldPositionTrace_;
    NetworkValue<Quaternion> worldRotationTrace_;
    /// @}
};

/// NetworkObject that is replicated on the client from prefab and is not updated afterwards.
/// Note: object position in the hierarchy of NetworkObject-s is still maintained.
class URHO3D_API StaticNetworkObject : public NetworkObject
{
    URHO3D_OBJECT(StaticNetworkObject, NetworkObject);

public:
    /// Masks for reliable update.
    /// @{
    static constexpr unsigned ParentObjectMask = 1 << 0;
    /// @}

    explicit StaticNetworkObject(Context* context);
    ~StaticNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Attribute modification. Don't do that after replication!
    /// @{
    void SetClientPrefab(XMLFile* prefab);
    /// @}

    /// Implementation of NetworkObject
    /// @{
    void InitializeOnServer() override;

    void WriteSnapshot(unsigned frame, Serializer& dest) override;
    unsigned GetReliableDeltaMask(unsigned frame) override;
    void WriteReliableDelta(unsigned frame, unsigned mask, Serializer& dest) override;

    void ReadSnapshot(unsigned frame, Deserializer& src) override;
    void ReadReliableDelta(unsigned frame, Deserializer& src) override;
    /// @}

protected:
    ResourceRef GetClientPrefabAttr() const;
    void SetClientPrefabAttr(const ResourceRef& value);

private:
    SharedPtr<XMLFile> clientPrefab_;

    NetworkId latestSentParentObject_{InvalidNetworkId};
};

/// Aspect of network behavior that is injected into BehaviorNetworkObject.
/// NetworkBehavior should be created only after owner BehaviorNetworkObject is created,
/// but before it's replicated to clients (on server) or creation is finished (on client).
/// This basically means that list of NetworkBehavior-s attached to BehaviorNetworkObject
/// should stay the same during all lifetime of BehaviorNetworkObject.
class URHO3D_API NetworkBehavior : public Component
{
    URHO3D_OBJECT(NetworkBehavior, Component);

public:
    explicit NetworkBehavior(Context* context);
    ~NetworkBehavior() override;

    static void RegisterObject(Context* context);

    /// Internal. Set owner NetworkObject.
    void SetNetworkObject(BehaviorNetworkObject* owner);

    /// Return owner NetworkObject.
    BehaviorNetworkObject* GetNetworkObject() const { return owner_; }

    /// Callbacks from BehaviorNetworkObject.
    /// @{
    virtual void InitializeOnServer() {}
    virtual void UpdateTransformOnServer() {}
    virtual void WriteSnapshot(unsigned frame, Serializer& dest) {}
    virtual unsigned GetReliableDeltaMask(unsigned frame) { return 0; }
    virtual void WriteReliableDelta(unsigned frame, unsigned mask, Serializer& dest) {}
    virtual unsigned GetUnreliableDeltaMask(unsigned frame) { return 0; }
    virtual void WriteUnreliableDelta(unsigned frame, unsigned mask, Serializer& dest) {}
    virtual void ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src) {}

    virtual void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame) {}
    virtual void ReadSnapshot(unsigned frame, Deserializer& src) {} // TODO(network): Rename to InitializeFromSnapshot?
    virtual void ReadReliableDelta(unsigned frame, Deserializer& src) {}
    virtual void ReadUnreliableDelta(unsigned frame, Deserializer& src) {}
    virtual void OnUnreliableDelta(unsigned frame) {}
    virtual unsigned GetUnreliableFeedbackMask(unsigned frame) { return 0; }
    virtual void WriteUnreliableFeedback(unsigned frame, unsigned mask, Serializer& dest) {}
    /// @}

protected:
    /// Component implementation
    /// @{
    void OnNodeSet(Node* node) override;
    /// @}

private:
    WeakPtr<BehaviorNetworkObject> owner_;
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

    /// Implementation of NetworkObject
    /// @{
    void InitializeOnServer() override;
    void UpdateTransformOnServer() override;
    void WriteSnapshot(unsigned frame, Serializer& dest) override;
    unsigned GetReliableDeltaMask(unsigned frame) override;
    void WriteReliableDelta(unsigned frame, unsigned mask, Serializer& dest) override;
    unsigned GetUnreliableDeltaMask(unsigned frame) override;
    void WriteUnreliableDelta(unsigned frame, unsigned mask, Serializer& dest) override;
    void ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src) override;

    void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame) override;
    void ReadSnapshot(unsigned frame, Deserializer& src) override;
    void ReadReliableDelta(unsigned frame, Deserializer& src) override;
    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override;
    unsigned GetUnreliableFeedbackMask(unsigned frame) override;
    void WriteUnreliableFeedback(unsigned frame, unsigned mask, Serializer& dest) override;
    /// @}

private:
    void InitializeBehaviors();

    struct ConnectedNetworkBehavior
    {
        unsigned bit_{};
        WeakPtr<NetworkBehavior> component_;
        unsigned tempMask_{};
    };

    ea::vector<ConnectedNetworkBehavior> behaviors_;
    unsigned tempMask_{};
};

/// Behavior that replicates transform of the node.
class URHO3D_API ReplicatedNetworkTransform : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedNetworkTransform, NetworkBehavior);

public:
    static const unsigned NumUploadAttempts = 8;

    explicit ReplicatedNetworkTransform(Context* context);
    ~ReplicatedNetworkTransform() override;

    static void RegisterObject(Context* context);

    void SetTrackOnly(bool value) { trackOnly_ = value; }
    bool GetTrackOnly() const { return trackOnly_; }

    /// Implement NetworkBehavior.
    /// @{
    void InitializeOnServer() override;
    void UpdateTransformOnServer() override;
    unsigned GetUnreliableDeltaMask(unsigned frame) override;
    void WriteUnreliableDelta(unsigned frame, unsigned mask, Serializer& dest) override;

    void ReadSnapshot(unsigned frame, Deserializer& src) override;
    void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame) override;
    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override;
    /// @}

    /// Getters for network properties
    /// @{
    Vector3 GetTemporalWorldPosition(const NetworkTime& time) const { return worldPositionTrace_.SampleValid(time); }
    Quaternion GetTemporalWorldRotation(const NetworkTime& time) const { return worldRotationTrace_.SampleValid(time); }
    ea::optional<Vector3> GetRawTemporalWorldPosition(unsigned frame) const { return worldPositionTrace_.GetRaw(frame); }
    ea::optional<Quaternion> GetRawTemporalWorldRotation(unsigned frame) const { return worldRotationTrace_.GetRaw(frame); }
    /// @}

private:
    bool trackOnly_{};

    unsigned pendingUploadAttempts_{};

    NetworkValue<Vector3> worldPositionTrace_;
    NetworkValue<Quaternion> worldRotationTrace_;
};

};
