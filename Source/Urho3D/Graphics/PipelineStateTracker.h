// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Container/Ptr.h>

#include <atomic>
#include <EASTL/vector.h>
#include <EASTL/algorithm.h>

namespace Urho3D
{

class PipelineStateTracker;

/// Holds subscription from one PipelineStateTracker to another. Neither checks nor affects objects lifetime.
class URHO3D_API PipelineStateSubscription
{
public:
    /// Construct default.
    PipelineStateSubscription() = default;

    /// Construct valid.
    PipelineStateSubscription(PipelineStateTracker* sender, PipelineStateTracker* subscriber) { Reset(sender, subscriber); }

    /// Copy-construct.
    PipelineStateSubscription(const PipelineStateSubscription& other) { Reset(other.sender_, other.subscriber_); }

    /// Move-construct.
    PipelineStateSubscription(PipelineStateSubscription&& other)
    {
        ea::swap(sender_, other.sender_);
        ea::swap(subscriber_, other.subscriber_);
    }

    /// Destruct.
    ~PipelineStateSubscription() { Reset(nullptr, nullptr); }

    /// Assign.
    PipelineStateSubscription& operator =(const PipelineStateSubscription& other)
    {
        Reset(other.sender_, other.subscriber_);
        return *this;
    }

    /// Move-assign.
    PipelineStateSubscription& operator =(PipelineStateSubscription&& other)
    {
        ea::swap(sender_, other.sender_);
        ea::swap(subscriber_, other.subscriber_);
        return *this;
    }

private:
    /// Reset and update dependency.
    void Reset(PipelineStateTracker* sender, PipelineStateTracker* subscriber);

    /// Owned pointer.
    PipelineStateTracker* sender_{};
    /// Owner.
    PipelineStateTracker* subscriber_{};
};

/// Helper class to track pipeline state changes caused by derived class.
class URHO3D_API PipelineStateTracker
{
    friend class PipelineStateSubscription;

public:
    /// Destruct.
    virtual ~PipelineStateTracker();
    /// Return (partial) pipeline state hash. Save to call from multiple threads as long as the object is not changing.
    unsigned GetPipelineStateHash() const
    {
        const unsigned hash = pipelineStateHash_.load(std::memory_order_relaxed);
        if (hash)
            return hash;

        const unsigned newHash = ea::max(1u, RecalculatePipelineStateHash());
        pipelineStateHash_.store(newHash, std::memory_order_relaxed);
        return newHash;
    }

    /// Mark pipeline state hash as dirty.
    void MarkPipelineStateHashDirty();

protected:
    /// Create dependency onto another pipeline state.
    PipelineStateSubscription CreateDependency(PipelineStateTracker* dependency);
    /// Add reference to subscriber pipeline state tracker.
    void AddSubscriberReference(PipelineStateTracker* subscriber);
    /// Remove reference to subscriber pipeline state tracker.
    void RemoveSubscriberReference(PipelineStateTracker* subscriber);

private:
    /// Vector of subscribers with reference counters.
    using DependantVector = ea::vector<ea::pair<PipelineStateTracker*, unsigned>>;
    /// Recalculate hash (must not be non zero). Shall be save to call from multiple threads as long as the object is not changing.
    virtual unsigned RecalculatePipelineStateHash() const = 0;
    /// Find subscriber iterator by pointer.
    DependantVector::iterator FindSubscriberIter(PipelineStateTracker* subscriber);

    /// Cached hash.
    mutable std::atomic_uint32_t pipelineStateHash_{0};
    /// Other pipeline state trackers depending on this tracker.
    DependantVector subscribers_;
};

}
