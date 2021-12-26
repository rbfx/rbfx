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

/// Default implementation of NetworkObject that does some basic replication.
class URHO3D_API DefaultNetworkObject : public NetworkObject
{
    URHO3D_OBJECT(DefaultNetworkObject, NetworkObject);

public:
    enum : unsigned
    {
        UpdateParentNetworkObjectId = 1 << 0,
        UpdateWorldTransform = 1 << 1
    };

    explicit DefaultNetworkObject(Context* context);
    ~DefaultNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Implementation of NetworkObject
    /// @{
    void InitializeOnServer() override;

    void OnTransformDirty() override;
    void WriteSnapshot(unsigned frame, VectorBuffer& dest) override;
    bool WriteReliableDelta(unsigned frame, VectorBuffer& dest) override;
    bool WriteUnreliableDelta(unsigned frame, VectorBuffer& dest) override;

    void InterpolateState(const NetworkTime& time) override;
    void ReadSnapshot(unsigned frame, VectorBuffer& src) override;
    void ReadReliableDelta(unsigned frame, VectorBuffer& src) override;
    void ReadUnreliableDelta(unsigned frame, VectorBuffer& src) override;
    /// @}

protected:
    /// Evaluates the mask for reliable delta update. Called exactly once by WriteReliableDelta.
    virtual unsigned EvaluateReliableDeltaMask();
    /// Returns the mask for unreliable delta update. Called exactly once by WriteUnreliableDelta.
    virtual unsigned EvaluateUnreliableDeltaMask();

private:
    static const unsigned WorldTransformCooldown = 5;

    /// Delta update caches (for server)
    /// @{
    NetworkId lastParentNetworkId_{ InvalidNetworkId };
    unsigned worldTransformCounter_{};
    /// @}

    /// Tracers for synchronized values (for client)
    /// @{
    NetworkValue<Vector3> worldPositionTrace_;
    NetworkValue<Quaternion> worldRotationTrace_;
    /// @}
};

}
