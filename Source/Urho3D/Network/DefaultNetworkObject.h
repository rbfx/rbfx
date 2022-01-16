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

#include "../Network/NetworkObject.h"
#include "../Network/NetworkValue.h"

namespace Urho3D
{

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

    void OnTransformDirty() override;
    void WriteSnapshot(unsigned frame, Serializer& dest) override;
    bool WriteReliableDelta(unsigned frame, Serializer& dest) override;
    bool WriteUnreliableDelta(unsigned frame, Serializer& dest) override;

    void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, bool isNewInputFrame) override;
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

}
