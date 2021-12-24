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

#include "../Core/Assert.h"
#include "../Container/FlagSet.h"
#include "../Scene/Component.h"
#include "../Network/NetworkManager.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>

namespace Urho3D
{

class AbstractConnection;

/// Base component of Network-replicated object.
///
/// Each NetworkObject has ID unique within the owner Scene.
/// Derive from NetworkObject to have custom network logic.
/// Don't create more than one NetworkObject per Node.
///
/// Hierarchy is updated after NetworkObject node is dirtied.
class URHO3D_API NetworkObject : public Component
{
    URHO3D_OBJECT(NetworkObject, Component);

public:
    explicit NetworkObject(Context* context);
    ~NetworkObject() override;

    static void RegisterObject(Context* context);

    /// Update pointer to the parent NetworkObject.
    void UpdateParent();
    /// Assign NetworkId. On the Server, it's better to let the Server assign ID, to avoid unwanted side effects.
    void SetNetworkId(NetworkId networkId) { networkId_ = networkId; }
    /// Return current or last NetworkId. Return InvalidNetworkId if not registered.
    NetworkId GetNetworkId() const { return networkId_; }
    /// Return NetworkId of parent NetworkObject.
    NetworkId GetParentNetworkId() const { return parentNetworkObject_ ? parentNetworkObject_->GetNetworkId() : InvalidNetworkId; }
    /// Return parent NetworkObject.
    NetworkObject* GetParentNetworkObject() const { return parentNetworkObject_; }
    /// Return children NetworkObject.
    const auto& GetChildrenNetworkObjects() const { return childrenNetworkObjects_; }

    /// Called on server side only. ServerNetworkManager is guaranteed to be available.
    /// @{

    /// Return whether the component should be replicated for specified client connection.
    virtual bool IsRelevantForClient(AbstractConnection* connection);
    /// Perform server-side initialization. Called once.
    virtual void InitializeOnServer();
    /// Called when transform of the object is dirtied.
    virtual void OnTransformDirty();
    /// Write full snapshot on server.
    virtual void WriteSnapshot(VectorBuffer& dest);
    /// Write reliable delta update on server. Delta is applied to previous delta or snapshot message.
    virtual bool WriteReliableDelta(VectorBuffer& dest);
    /// Write unreliable delta update on server.
    virtual bool WriteUnreliableDelta(VectorBuffer& dest);

    /// @}

    /// Called on client side only. ClientNetworkManager is guaranteed to be available and synchronized.
    /// @{

    /// Interpolate replicated state.
    virtual void InterpolateState(unsigned currentFrame, float blendFactor);
    /// Prepare to this compnent being removed by the authority of the server.
    virtual void PrepareToRemove();
    /// Read full snapshot.
    virtual void ReadSnapshot(unsigned timestamp, VectorBuffer& src);
    /// Read reliable delta update. Delta is applied to previous reliable delta or snapshot message.
    virtual void ReadReliableDelta(unsigned timestamp, VectorBuffer& src);
    /// Read unreliable delta update.
    virtual void ReadUnreliableDelta(unsigned timestamp, VectorBuffer& src);

    /// @}

protected:
    /// Component implementation
    /// @{
    void OnNodeSet(Node* node) override;
    void OnMarkedDirty(Node* node) override;
    /// @}

    NetworkObject* GetOtherNetworkObject(NetworkId networkId) const;
    void SetParentNetworkObject(NetworkId parentNetworkId);
    ClientNetworkManager* GetClientNetworkManager() const;
    ServerNetworkManager* GetServerNetworkManager() const;

private:
    void UpdateCurrentScene(Scene* scene);
    NetworkObject* FindParentNetworkObject() const;
    void AddChildNetworkObject(NetworkObject* networkObject);
    void RemoveChildNetworkObject(NetworkObject* networkObject);

    /// NetworkManager corresponding to the NetworkObject.
    WeakPtr<NetworkManager> networkManager_;
    /// Network ID, unique within Scene.
    /// May contain outdated value if NetworkObject is not registered in any NetworkManager.
    NetworkId networkId_{InvalidNetworkId};

    /// NetworkObject hierarchy
    /// @{
    WeakPtr<NetworkObject> parentNetworkObject_;
    ea::vector<WeakPtr<NetworkObject>> childrenNetworkObjects_;
    /// @}
};

/// Helper utility to trace values of the entity at multiple time points.
/// Frame is assumed to grow steady. Last frame is always valid.
template <class T>
class ValueTrace
{
public:
    using Container = ea::vector<ea::optional<T>>;

    /// Resize container preserving contents when possible.
    void Resize(unsigned capacity)
    {
        URHO3D_ASSERT(capacity > 0);

        const unsigned oldCapacity = values_.size();
        if (capacity == oldCapacity)
        {
            return;
        }
        else if (oldCapacity == 0)
        {
            // Initialize normally first time
            values_.resize(capacity);
        }
        else if (capacity > oldCapacity)
        {
            // Insert new elements after last one, it will not affect index of last one
            values_.insert(ea::next(values_.begin(), lastIndex_ + 1), capacity - oldCapacity, ea::nullopt);
        }
        else
        {
            // Remove elements starting from the old ones
            values_ = AsVector();
            values_.erase(values_.begin(), ea::next(values_.begin(), oldCapacity - capacity));
            lastIndex_ = values_.size() - 1;
        }
    }

    /// Set value for given frame if not set. No op otherwise.
    void Append(unsigned frame, const T& value) { SetInternal(frame, value, false); }

    /// Set value for given frame. Overwrites previous value.
    void Replace(unsigned frame, const T& value) { SetInternal(frame, value, true); }

    /// Extrapolate value into specified frame if it's uninitialized.
    /// Return true if new value was filled.
    bool ExtrapolateIfEmpty(unsigned frame)
    {
        const auto nearestValidFrame = GetNearestValidFrame(frame);
        if (!nearestValidFrame || *nearestValidFrame == frame)
            return false;

        const auto value = GetValue(*nearestValidFrame);
        URHO3D_ASSERT(value);
        Append(frame, *value);
        return true;
    }

    /// If called, interpolation is guaranteed to stay continuous regardless of appended values.
    bool PrepareForInterpolationTo(unsigned frame)
    {
        bool result = false;
        result |= ExtrapolateIfEmpty(frame - 1);
        result |= ExtrapolateIfEmpty(frame);
        return result;
    }

    /// Return as normal vector of values. The last element is the latest appended value.
    Container AsVector() const
    {
        const unsigned capacity = values_.size();

        Container result(capacity);
        for (unsigned i = 0; i < capacity; ++i)
            result[i] = values_[(lastIndex_ + i + 1) % capacity];
        return result;
    }

    /// Return raw value or null if nothing is stored.
    ea::optional<T> GetValue(unsigned frame) const
    {
        const unsigned capacity = values_.size();
        const int behind = static_cast<int>(lastFrame_ - frame);
        if (behind < 0 || behind >= capacity)
            return ea::nullopt;

        const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
        return values_[wrappedIndex];
    }

    /// Return specified frame, or nearest frame with valid value prior to specified one.
    ea::optional<unsigned> GetNearestValidFrame(unsigned frame) const
    {
        const unsigned capacity = values_.size();
        const int offset = ea::max(0, static_cast<int>(lastFrame_ - frame));
        if (offset >= capacity)
            return ea::nullopt;

        for (unsigned behind = offset; behind < capacity; ++behind)
        {
            const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
            const auto& value = values_[wrappedIndex];
            if (value)
                return lastFrame_ - behind;
        }
        return ea::nullopt;
    }

    /// Sample value at given frame and blend factor. Requires two consecutive values.
    ea::optional<T> GetBlendedValue(unsigned frame, float blendFactor) const
    {
        const auto value1 = GetValue(frame);
        const auto value2 = GetValue(frame + 1);
        if (!value1 || !value2)
            return ea::nullopt;

        return InterpolateOnNetworkClient(*value1, *value2, blendFactor);
    }

private:
    void SetInternal(unsigned frame, const T& value, bool overwrite)
    {
        URHO3D_ASSERT(!values_.empty());

        // Initialize first frame
        if (!initialized_)
        {
            initialized_ = true;
            lastFrame_ = frame;
            lastIndex_ = 0;
            values_[0] = value;
            return;
        }

        const unsigned capacity = values_.size();
        const int offset = static_cast<int>(frame - lastFrame_);

        if (offset > 0)
        {
            // Roll buffer forward on positive delta, resetting intermediate values
            lastFrame_ += offset;
            lastIndex_ = (lastIndex_ + offset) % capacity;
            values_[lastIndex_] = value;

            const unsigned uninitializedBeginBehind = 1;
            const unsigned uninitializedEndBehind = ea::min(static_cast<unsigned>(offset), capacity);
            for (unsigned behind = uninitializedBeginBehind; behind < uninitializedEndBehind; ++behind)
            {
                const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
                values_[wrappedIndex] = ea::nullopt;
            }
        }
        else
        {
            // Set singular past value, no overwrite
            const unsigned behind = -offset;
            if (behind < capacity)
            {
                const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
                if (overwrite || !values_[wrappedIndex])
                    values_[wrappedIndex] = value;
            }
        }
    }

    bool initialized_{};
    unsigned lastFrame_{};
    unsigned lastIndex_{};
    Container values_;
};

template <class T>
T InterpolateOnNetworkClient(const T& lhs, const T& rhs, float blendFactor)
{
    return Lerp(lhs, rhs, blendFactor);
}

inline Quaternion InterpolateOnNetworkClient(const Quaternion& lhs, const Quaternion& rhs, float blendFactor)
{
    return lhs.Slerp(rhs, blendFactor);
}

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
    void WriteSnapshot(VectorBuffer& dest) override;
    bool WriteReliableDelta(VectorBuffer& dest) override;
    bool WriteUnreliableDelta(VectorBuffer& dest) override;

    void InterpolateState(unsigned currentFrame, float blendFactor) override;
    void ReadSnapshot(unsigned timestamp, VectorBuffer& src) override;
    void ReadReliableDelta(unsigned timestamp, VectorBuffer& src) override;
    void ReadUnreliableDelta(unsigned timestamp, VectorBuffer& src) override;
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
    ValueTrace<Vector3> worldPositionTrace_;
    ValueTrace<Quaternion> worldRotationTrace_;
    /// @}
};

}
