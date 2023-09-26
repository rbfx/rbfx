// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderPool.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/RawBuffer.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

RenderPool::RenderPool(RenderDevice* renderDevice)
    : Object(renderDevice->GetContext())
    , renderDevice_(renderDevice)
{
    scratchBuffer_.resize(64 * 1024);
}

RenderPool::~RenderPool()
{
}

void RenderPool::Invalidate()
{
    uniformBuffers_.clear();
}

void RenderPool::Restore()
{
}

void RenderPool::OnFrameEnd()
{
    RecycleTextures();
}

RawBuffer* RenderPool::GetUniformBuffer(unsigned id, unsigned size)
{
    const unsigned sizeQuant = 512;
    size = (size + sizeQuant - 1) / sizeQuant * sizeQuant;

    const unsigned long long key = (static_cast<unsigned long long>(id) << 32) | size;
    auto& uniformBuffer = uniformBuffers_[key];

    if (!uniformBuffer)
    {
        RawBufferParams params;
        params.type_ = BufferType::Uniform;
        params.size_ = size;
        params.flags_ = BufferFlag::Dynamic | BufferFlag::Discard;

        uniformBuffer = ea::make_unique<RawBuffer>(context_, params);
    }
    return uniformBuffer.get();
}

RawTexture* RenderPool::GetTexture(const RawTextureParams& params, const void* persistenceKey)
{
    return persistenceKey ? GetPersistentTexture(params, persistenceKey) : GetTransientTexture(params);
}

long long RenderPool::GetAge(const TextureCacheEntry& entry) const
{
    const FrameIndex currentFrame = renderDevice_->GetFrameIndex();
    return static_cast<long long>(currentFrame) - static_cast<long long>(entry.lastUsedFrame_);
}

RawTexture* RenderPool::GetTransientTexture(const RawTextureParams& params)
{
    TextureCacheEntryGroup& allocations = transientTextures_[params];
    if (allocations.numUsed_ < allocations.entries_.size())
    {
        TextureCacheEntry& entry = allocations.entries_[allocations.numUsed_];

        ++allocations.numUsed_;
        entry.lastUsedFrame_ = renderDevice_->GetFrameIndex();

        return entry.texture_.get();
    }

    TextureCacheEntry& entry = allocations.entries_.emplace_back();

    ++allocations.numUsed_;
    entry.texture_ = ea::make_unique<RawTexture>(context_, params);
    entry.lastUsedFrame_ = renderDevice_->GetFrameIndex();
    ++numAddedTextures_;

    return entry.texture_.get();
}

RawTexture* RenderPool::GetPersistentTexture(const RawTextureParams& params, const void* persistenceKey)
{
    TextureCacheEntry& entry = persistentTextures_[ea::make_pair(params, persistenceKey)];
    entry.lastUsedFrame_ = renderDevice_->GetFrameIndex();
    if (!entry.texture_)
    {
        entry.texture_ = ea::make_unique<RawTexture>(context_, params);
        ++numAddedTextures_;
    }
    return entry.texture_.get();
}

void RenderPool::RecycleTextures()
{
    CleanupOldTextures();
    CleanupTexturesExceedingQuota();

    const FrameIndex currentFrame = renderDevice_->GetFrameIndex();
    const auto framesSinceLastLog = static_cast<long long>(currentFrame) - static_cast<long long>(lastLogFrame_);
    if ((numAddedTextures_ > 0 || numRemovedTextures_ > 0) && framesSinceLastLog >= settings_.logPeriod_)
    {
        unsigned totalTextures = 0;
        for (const auto& [_, allocations] : transientTextures_)
            totalTextures += allocations.entries_.size();
        totalTextures += persistentTextures_.size();

        URHO3D_LOGDEBUG("RenderPool: {} uniform buffers, {} textures (+{} -{})", uniformBuffers_.size(), totalTextures,
            numAddedTextures_, numRemovedTextures_);

        lastLogFrame_ = currentFrame;
        numAddedTextures_ = 0;
        numRemovedTextures_ = 0;
    }
}

void* RenderPool::AllocateScratchBuffer(unsigned size)
{
    size = (size + 15) & ~15;

    if (scratchBufferOffset_ + size > scratchBuffer_.size())
    {
        void* buffer = malloc(size);
        temporaryScratchBufferAllocations_.push_back(buffer);
        temporaryScratchBufferAllocationsSize_ += size;
        return buffer;
    }
    else
    {
        void* buffer = scratchBuffer_.data() + scratchBufferOffset_;
        scratchBufferAllocations_.push_back(buffer);
        scratchBufferOffset_ += size;
        return buffer;
    }
}

void RenderPool::ReleaseScratchBuffer(void* buffer)
{
    const bool isTemporary = temporaryScratchBufferAllocations_.contains(buffer);
    const bool isStandard = scratchBufferAllocations_.contains(buffer);
    URHO3D_ASSERT(isTemporary != isStandard);

    if (isTemporary)
    {
        free(buffer);
        temporaryScratchBufferAllocations_.erase_first(buffer);
    }
    else if (isStandard)
    {
        scratchBufferAllocations_.erase_first(buffer);
    }

    const bool isLastAllocation = scratchBufferAllocations_.empty() && temporaryScratchBufferAllocations_.empty();
    if (isLastAllocation)
    {
        if (temporaryScratchBufferAllocationsSize_ != 0)
        {
            const unsigned newCapacity = (scratchBuffer_.size() + temporaryScratchBufferAllocationsSize_) * 3 / 2;
            scratchBuffer_.resize(newCapacity);
        }
        scratchBufferOffset_ = 0;
        temporaryScratchBufferAllocationsSize_ = 0;
    }
}

void RenderPool::CleanupOldTextures()
{
    for (auto& [_, allocations] : transientTextures_)
    {
        allocations.numUsed_ = 0;
        for (TextureCacheEntry& entry : allocations.entries_)
        {
            const unsigned oldSize = allocations.entries_.size();
            ea::erase_if(allocations.entries_,
                [&](const auto& entry) { return GetAge(entry) >= settings_.textureCacheMaxFrames_; });
            numRemovedTextures_ += oldSize - allocations.entries_.size();
        }
    }
    ea::erase_if(transientTextures_, [&](const auto& pair) { return pair.second.entries_.empty(); });

    const unsigned oldSize = persistentTextures_.size();
    ea::erase_if(
        persistentTextures_, [&](const auto& pair) { return GetAge(pair.second) >= settings_.textureCacheMaxFrames_; });
    numRemovedTextures_ += oldSize - persistentTextures_.size();
}

void RenderPool::CleanupTexturesExceedingQuota()
{
    const FrameIndex currentFrame = renderDevice_->GetFrameIndex();

    unsigned totalTextures = 0;
    unsigned totalOldTextures = 0;

    for (const auto& [_, allocations] : transientTextures_)
    {
        for (const TextureCacheEntry& entry : allocations.entries_)
        {
            ++totalTextures;
            if (GetAge(entry) >= settings_.textureCacheMinFrames_)
                ++totalOldTextures;
        }
    }
    for (const auto& [_, entry] : persistentTextures_)
    {
        ++totalTextures;
        if (GetAge(entry) >= settings_.textureCacheMinFrames_)
            ++totalOldTextures;
    }

    const int maxOldTextures = CeilToInt(totalTextures * (1.0 - settings_.textureCacheMinLoadFactor_));
    if (totalOldTextures < maxOldTextures)
        return;

    for (auto& [_, allocations] : transientTextures_)
    {
        const unsigned oldSize = allocations.entries_.size();
        ea::erase_if(
            allocations.entries_, [&](const auto& entry) { return GetAge(entry) >= settings_.textureCacheMinFrames_; });
        numRemovedTextures_ += oldSize - allocations.entries_.size();
    }
    ea::erase_if(transientTextures_, [&](const auto& pair) { return pair.second.entries_.empty(); });

    const unsigned oldSize = persistentTextures_.size();
    ea::erase_if(
        persistentTextures_, [&](const auto& pair) { return GetAge(pair.second) >= settings_.textureCacheMinFrames_; });
    numRemovedTextures_ += oldSize - persistentTextures_.size();
}

} // namespace Urho3D
