// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/TransformedSpan.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Graphics/ReflectionProbeData.h"
#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderPipeline/RenderPipeline.h"
#include "Urho3D/Scene/Component.h"
#include "Urho3D/Scene/TrackedComponent.h"

#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class TextureCube;
class Viewport;

enum class CubemapUpdateStage
{
    Idle,
    RenderFaces,
    Ready
};

struct CubemapUpdateResult
{
    unsigned numRenderedFaces_{};
    bool isComplete_{};
};

struct CubemapRenderingSettings
{
    static constexpr unsigned DefaultTextureSize = 256;
    static constexpr unsigned DefaultViewMask = 0xffffffff;
    static constexpr float DefaultNearClip = 0.1f;
    static constexpr float DefaultFarClip = 100.0f;

    unsigned textureSize_{DefaultTextureSize};
    unsigned viewMask_{DefaultViewMask};
    float nearClip_{DefaultNearClip};
    float farClip_{DefaultFarClip};

    bool operator==(const CubemapRenderingSettings& rhs) const
    {
        return textureSize_ == rhs.textureSize_
            && viewMask_ == rhs.viewMask_
            && nearClip_ == rhs.nearClip_
            && farClip_ == rhs.farClip_;
    }
    bool operator!=(const CubemapRenderingSettings& rhs) const { return !(*this == rhs); }
};

struct CubemapUpdateParameters
{
    CubemapRenderingSettings settings_;
    Vector3 position_;
    bool slicedUpdate_{};
    bool filterResult_{};
    WeakPtr<TextureCube> overrideFinalTexture_;

    bool IsConsistentWith(const CubemapUpdateParameters& rhs) const
    {
        return settings_ == rhs.settings_
            && slicedUpdate_ == rhs.slicedUpdate_
            && filterResult_ == rhs.filterResult_
            && overrideFinalTexture_ == rhs.overrideFinalTexture_;
    }
};

/// Utility class that handles cubemap rendering from scene.
class CubemapRenderer : public Object
{
    URHO3D_OBJECT(CubemapRenderer, Object);

public:
    Signal<void(TextureCube* texture)> OnCubemapRendered;

    explicit CubemapRenderer(Scene* scene);
    ~CubemapRenderer() override;

    static void DefineTexture(
        TextureCube* texture, const CubemapRenderingSettings& settings, TextureFlags flags = TextureFlag::None);

    CubemapUpdateResult Update(const CubemapUpdateParameters& params);

private:
    struct CachedPipelineStates
    {
        unsigned numLevels_{};
        ea::vector<SharedPtr<PipelineState>> pipelineStates_;
    };

    void InitializeRenderPipeline();
    void InitializeCameras();

    void ConnectViewportsToTexture(TextureCube* texture);
    void DisconnectViewportsFromTexture(TextureCube* texture) const;

    void PrepareForUpdate(const CubemapUpdateParameters& params);
    bool IsTextureMatching(TextureCube* textureCube, const CubemapRenderingSettings& settings) const;
    CubemapUpdateResult UpdateFull();
    CubemapUpdateResult UpdateSliced();
    void QueueFaceUpdate(CubeMapFace face);

    void ProcessFaceRendered();
    void ProcessCubemapRendered();

    void EnsurePipelineStates(unsigned numLevels);
    void FilterCubemap(TextureCube* sourceTexture, TextureCube* destTexture);

    WeakPtr<Scene> scene_;
    ea::array<SharedPtr<Node>, MAX_CUBEMAP_FACES> renderCameras_;
    ea::array<SharedPtr<Viewport>, MAX_CUBEMAP_FACES> viewports_;
    SharedPtr<RenderPipeline> renderPipeline_;
    SharedPtr<TextureCube> viewportTexture_;
    SharedPtr<TextureCube> filteredTexture_;

    CubemapUpdateParameters currentParams_;
    CubemapUpdateStage updateStage_{};
    unsigned numFacesToUpdate_{};
    unsigned numFacesToRender_{};
    WeakPtr<TextureCube> currentViewportTexture_;
    WeakPtr<TextureCube> currentFilteredTexture_;
    bool viewportsConnectedToSelf_{};

    ea::optional<CachedPipelineStates> cachedPipelineStates_;
};

}
