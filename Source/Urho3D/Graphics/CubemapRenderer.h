//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Container/TransformedSpan.h"
#include "../Core/Signal.h"
#include "../Graphics/ReflectionProbeData.h"
#include "../Math/BoundingBox.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Scene/Component.h"
#include "../Scene/TrackedComponent.h"

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

struct CubemapRenderingParameters
{
    static constexpr unsigned DefaultTextureSize = 256;
    static constexpr unsigned DefaultViewMask = 0xffffffff;
    static constexpr float DefaultNearClip = 0.1f;
    static constexpr float DefaultFarClip = 100.0f;

    unsigned textureSize_{DefaultTextureSize};
    unsigned viewMask_{DefaultViewMask};
    float nearClip_{DefaultNearClip};
    float farClip_{DefaultFarClip};
};

/// Utility class that handles cubemap rendering from scene.
class CubemapRenderer : public Object
{
    URHO3D_OBJECT(CubemapRenderer, Object);

public:
    Signal<void(TextureCube* texture)> OnCubemapRendered;

    explicit CubemapRenderer(Scene* scene);
    ~CubemapRenderer() override;

    static void DefineTexture(TextureCube* texture, const CubemapRenderingParameters& params);

    void Define(const CubemapRenderingParameters& params);
    void OverrideOutputTexture(TextureCube* texture);
    CubemapUpdateResult Update(const Vector3& position, bool slicedUpdate);

    TextureCube* GetTexture() const { return renderTexture_; }

private:
    void InitializeRenderPipeline();
    void InitializeCameras();
    void InitializeTexture(const CubemapRenderingParameters& params);
    void ConnectViewportsToTexture(TextureCube* texture);
    void DisconnectViewportsFromTexture(TextureCube* texture) const;

    CubemapUpdateResult UpdateFull();
    CubemapUpdateResult UpdateSliced();
    void QueueFaceUpdate(CubeMapFace face);

    void ProcessFaceRendered();
    void ProcessCubemapRendered();

    WeakPtr<Scene> scene_;
    ea::array<SharedPtr<Node>, MAX_CUBEMAP_FACES> renderCameras_;
    ea::array<SharedPtr<Viewport>, MAX_CUBEMAP_FACES> viewports_;

    SharedPtr<RenderPipeline> renderPipeline_;
    SharedPtr<TextureCube> renderTexture_;
    WeakPtr<TextureCube> overrideTexture_;

    CubemapUpdateStage updateStage_{};
    unsigned numFacesToUpdate_{};
    unsigned numFacesToRender_{};
};

}
