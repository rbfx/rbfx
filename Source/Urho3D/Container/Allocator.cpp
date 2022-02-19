//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Container/Allocator.h"
#include "../Core/Profiler.h"

#if URHO3D_STATIC
URHO3D_API void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new(size);
}

URHO3D_API void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new(size);
}
#endif

#include "../DebugNew.h"

namespace Urho3D
{

AllocatorBlock* AllocatorReserveBlock(AllocatorBlock* allocator, unsigned nodeSize, unsigned capacity)
{
    URHO3D_PROFILE("AllocatorReserveBlock");

    if (!capacity)
        capacity = 1;

    auto* blockPtr = new unsigned char[sizeof(AllocatorBlock) + capacity * (sizeof(AllocatorNode) + nodeSize)];
    auto* newBlock = reinterpret_cast<AllocatorBlock*>(blockPtr);
    newBlock->nodeSize_ = nodeSize;
    newBlock->capacity_ = capacity;
    newBlock->free_ = nullptr;
    newBlock->next_ = nullptr;

    if (!allocator)
        allocator = newBlock;
    else
    {
        newBlock->next_ = allocator->next_;
        allocator->next_ = newBlock;
    }

    // Initialize the nodes. Free nodes are always chained to the first (parent) allocator
    unsigned char* nodePtr = blockPtr + sizeof(AllocatorBlock);
    auto* firstNewNode = reinterpret_cast<AllocatorNode*>(nodePtr);

    for (unsigned i = 0; i < capacity - 1; ++i)
    {
        auto* newNode = reinterpret_cast<AllocatorNode*>(nodePtr);
        newNode->next_ = reinterpret_cast<AllocatorNode*>(nodePtr + sizeof(AllocatorNode) + nodeSize);
        nodePtr += sizeof(AllocatorNode) + nodeSize;
    }
    // i == capacity - 1
    {
        auto* newNode = reinterpret_cast<AllocatorNode*>(nodePtr);
        newNode->next_ = nullptr;
    }

    allocator->free_ = firstNewNode;

    return newBlock;
}

AllocatorBlock* AllocatorInitialize(unsigned nodeSize, unsigned initialCapacity)
{
    URHO3D_PROFILE("AllocatorInitialize");

    AllocatorBlock* block = AllocatorReserveBlock(nullptr, nodeSize, initialCapacity);
    return block;
}

void AllocatorUninitialize(AllocatorBlock* allocator)
{
    URHO3D_PROFILE("AllocatorUninitialize");

    while (allocator)
    {
        AllocatorBlock* next = allocator->next_;
        delete[] reinterpret_cast<unsigned char*>(allocator);
        allocator = next;
    }
}

void* AllocatorReserve(AllocatorBlock* allocator)
{
    if (!allocator)
        return nullptr;

    URHO3D_PROFILE("AllocatorReserve");

    if (!allocator->free_)
    {
        // Free nodes have been exhausted. Allocate a new larger block
        unsigned newCapacity = (allocator->capacity_ + 1) >> 1u;
        AllocatorReserveBlock(allocator, allocator->nodeSize_, newCapacity);
        allocator->capacity_ += newCapacity;
    }

    // We should have new free node(s) chained
    AllocatorNode* freeNode = allocator->free_;
    void* ptr = (reinterpret_cast<unsigned char*>(freeNode)) + sizeof(AllocatorNode);
    allocator->free_ = freeNode->next_;
    freeNode->next_ = nullptr;

    return ptr;
}

void AllocatorFree(AllocatorBlock* allocator, void* ptr)
{
    if (!allocator || !ptr)
        return;

    URHO3D_PROFILE("AllocatorFree");

    auto* dataPtr = static_cast<unsigned char*>(ptr);
    auto* node = reinterpret_cast<AllocatorNode*>(dataPtr - sizeof(AllocatorNode));

    // Chain the node back to free nodes
    node->next_ = allocator->free_;
    allocator->free_ = node;
}

}
