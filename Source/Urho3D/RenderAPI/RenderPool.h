// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class RawBuffer;
class RenderDevice;

struct RenderPoolSettings
{
    unsigned textureCacheMinFrames_{1};
    unsigned textureCacheMaxFrames_{1000};
    float textureCacheMinLoadFactor_{0.1f};
    unsigned logPeriod_{1000};
};

/// Pool for different resources used by renderer.
class URHO3D_API RenderPool : public Object
{
    URHO3D_OBJECT(RenderPool, Object);

public:
    explicit RenderPool(RenderDevice* renderDevice);
    ~RenderPool() override;

    void Invalidate();
    void Restore();
    void OnFrameEnd();

    /// Return uniform buffer.
    /// Buffers are recycled immediately, pass different ids to get different buffers.
    /// Don't store the pointer between frames.
    RawBuffer* GetUniformBuffer(unsigned id, unsigned size);

    /// Return texture.
    /// Transient textures are recycled on demand, same parameters will return different textures between recycles.
    /// Persistent textures are not recycled, same parameters will return same texture.
    RawTexture* GetTexture(const RawTextureParams& params, const void* persistenceKey = nullptr);

    /// Recycle all transient textures.
    void RecycleTextures();

    /// Allocate scratch buffer. Don't store the pointer between frames.
    void* AllocateScratchBuffer(unsigned size);
    /// Release scratch buffer.
    void ReleaseScratchBuffer(void* buffer);

private:
    struct TextureCacheEntry
    {
        ea::unique_ptr<RawTexture> texture_;
        FrameIndex lastUsedFrame_{};
    };
    struct TextureCacheEntryGroup
    {
        ea::vector<TextureCacheEntry> entries_;
        unsigned numUsed_{};
    };

    long long GetAge(const TextureCacheEntry& entry) const;
    RawTexture* GetTransientTexture(const RawTextureParams& params);
    RawTexture* GetPersistentTexture(const RawTextureParams& params, const void* persistenceKey);
    void CleanupOldTextures();
    void CleanupTexturesExceedingQuota();

    RenderDevice* renderDevice_{};
    RenderPoolSettings settings_;

    ea::unordered_map<unsigned long long, ea::unique_ptr<RawBuffer>> uniformBuffers_;
    ea::unordered_map<RawTextureParams, TextureCacheEntryGroup> transientTextures_;
    ea::unordered_map<ea::pair<RawTextureParams, const void*>, TextureCacheEntry> persistentTextures_;

    unsigned numAddedTextures_{};
    unsigned numRemovedTextures_{};
    FrameIndex lastLogFrame_{};

    ByteVector scratchBuffer_;
    unsigned scratchBufferOffset_{};
    ea::vector<void*> scratchBufferAllocations_;

    ea::vector<void*> temporaryScratchBufferAllocations_;
    unsigned temporaryScratchBufferAllocationsSize_{};
};

} // namespace Urho3D
