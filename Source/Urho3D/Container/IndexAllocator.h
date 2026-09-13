// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Mutex.h"

#include <EASTL/sort.h>
#include <EASTL/vector.h>

#include <atomic>

namespace Urho3D
{

/// Utility to assign unique non-zero IDs to objects. Thread-safe.
template <class T = DummyMutex>
class IndexAllocator
{
public:
    using MutexType = T;

    /// Return upper bound of allocated indices.
    unsigned GetNextFreeIndex() const { return nextIndex_.load(std::memory_order_relaxed); }

    /// Return number of currently allocated indices.
    unsigned GetSize() const
    {
        MutexLock<MutexType> lock(mutex_);
        return GetNextFreeIndex() - unusedIndices_.size();
    }

    /// Allocate index.
    unsigned Allocate()
    {
        MutexLock<MutexType> lock(mutex_);

        unsigned index{};
        if (unusedIndices_.empty())
        {
            index = GetNextFreeIndex();
            nextIndex_.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            index = unusedIndices_.back();
            unusedIndices_.pop_back();
        }

        return index;
    }

    /// Release index. Index should be previously returned from Allocate and not released yet.
    void Release(unsigned index)
    {
        MutexLock<MutexType> lock(mutex_);

        if (index + 1 == GetNextFreeIndex())
            nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        else
            unusedIndices_.push_back(index);
    }

    /// Shrink collection to minimum possible size preserving currently allocated indices.
    void Shrink()
    {
        MutexLock<MutexType> lock(mutex_);

        ea::sort(unusedIndices_.begin(), unusedIndices_.end());
        while (!unusedIndices_.empty() && unusedIndices_.back() + 1 == GetNextFreeIndex())
        {
            unusedIndices_.pop_back();
            nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        }
        unusedIndices_.shrink_to_fit();
    }

    /// Reset to default state.
    void Clear()
    {
        MutexLock<MutexType> lock(mutex_);
        nextIndex_ = 1;
        unusedIndices_.clear();
    }

private:
    /// Mutex that protects list and index.
    mutable MutexType mutex_;
    /// Next unused index.
    std::atomic_uint32_t nextIndex_{ 1 };
    /// Unused indices.
    ea::vector<unsigned> unusedIndices_;
};

/// Family of unique indices for template type.
template <class T>
class IDFamily
{
public:
    /// Construct.
    IDFamily() { AcquireObjectID(); }

    /// Destruct.
    ~IDFamily() { ReleaseObjectID(); }

    /// Return unique object ID or 0 if not assigned.
    unsigned GetObjectID() const { return objectId_; }

    /// Return upper bound of all used object IDs within family.
    static const unsigned GetNextFreeObjectID() { return indexAllocator.GetNextFreeIndex(); }

    /// Acquire unique object ID. Ignored if unique ID is already acquired.
    void AcquireObjectID()
    {
        if (!objectId_)
            objectId_ = indexAllocator.Allocate();
    }

    /// Release unique object ID. Ignored if unique ID is already released.
    void ReleaseObjectID()
    {
        if (objectId_)
        {
            indexAllocator.Release(objectId_);
            objectId_ = 0;
        }
    }

private:
    /// Shared allocator for this family.
    static IndexAllocator<Mutex> indexAllocator;
    /// Unique object ID.
    unsigned objectId_{};
};

template <class T> IndexAllocator<Mutex> IDFamily<T>::indexAllocator;

}
