// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/NonCopyable.h"

#include <Urho3D/Urho3D.h>

#include <cstddef>
#include <EASTL/utility.h>


namespace Urho3D
{

struct AllocatorBlock;
struct AllocatorNode;

/// %Allocator memory block.
struct AllocatorBlock
{
    /// Size of a node.
    unsigned nodeSize_;
    /// Number of nodes in this block.
    unsigned capacity_;
    /// First free node.
    AllocatorNode* free_;
    /// Next allocator block.
    AllocatorBlock* next_;
    /// Nodes follow.
};

/// %Allocator node.
struct AllocatorNode
{
    /// Next free node.
    AllocatorNode* next_;
    /// Data follows.
};

/// Initialize a fixed-size allocator with the node size and initial capacity.
URHO3D_API AllocatorBlock* AllocatorInitialize(unsigned nodeSize, unsigned initialCapacity = 1);
/// Uninitialize a fixed-size allocator. Frees all blocks in the chain.
URHO3D_API void AllocatorUninitialize(AllocatorBlock* allocator);
/// Reserve a node. Creates a new block if necessary.
URHO3D_API void* AllocatorReserve(AllocatorBlock* allocator);
/// Free a node. Does not free any blocks.
URHO3D_API void AllocatorFree(AllocatorBlock* allocator, void* ptr);

/// %Allocator template class. Allocates objects of a specific class.
template <class T> class Allocator : private NonCopyable
{
public:
    /// Construct.
    explicit Allocator(unsigned initialCapacity = 0) :
        allocator_(nullptr)
    {
        if (initialCapacity)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T), initialCapacity);
    }

    /// Destruct.
    ~Allocator()
    {
        AllocatorUninitialize(allocator_);
    }

    /// Reserve and default-construct an object.
    template<typename... Args>
    T* Reserve(Args&&... args)
    {
        if (!allocator_)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T));
        auto* newObject = static_cast<T*>(AllocatorReserve(allocator_));
        new(newObject) T(ea::forward<Args>(args)...);

        return newObject;
    }

    /// Reserve and copy-construct an object.
    T* Reserve(const T& object)
    {
        if (!allocator_)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T));
        auto* newObject = static_cast<T*>(AllocatorReserve(allocator_));
        new(newObject) T(object);

        return newObject;
    }

    /// Destruct and free an object.
    void Free(T* object)
    {
        (object)->~T();
        AllocatorFree(allocator_, object);
    }

private:
    /// Allocator block.
    AllocatorBlock* allocator_;
};

}
