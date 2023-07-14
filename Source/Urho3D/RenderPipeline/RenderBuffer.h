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

#pragma once

#include "../Container/FlagSet.h"
#include "../Core/Object.h"
#include "../Graphics/GraphicsDefs.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "Urho3D/RenderAPI/RenderTargetView.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class RawTexture;
class RenderSurface;
class RenderPipelineInterface;
struct CommonFrameInfo;
struct FrameInfo;

/// Base class fro writable texture or texture region. Readability is not guaranteed.
class URHO3D_API RenderBuffer : public Object
{
    URHO3D_OBJECT(RenderBuffer, Object);

public:
    /// Return readable texture. May return null if not supported.
    virtual RawTexture* GetTexture() const = 0;
    /// Return render surface. Face could be specified for cubemap texture.
    virtual RenderTargetView GetView(unsigned slice = 0) const = 0;
    virtual RenderTargetView GetReadOnlyDepthView(unsigned slice = 0) const = 0;
    /// Return effective viewport rectangle.
    /// Always equal to whole texture for TextureRenderBuffer, not so for viewport buffers.
    virtual IntRect GetViewportRect() const = 0;

protected:
    RenderBuffer(RenderPipelineInterface* renderPipeline);
    ~RenderBuffer() override;
    bool CheckIfBufferIsReady() const;

    /// RenderPipeline callbacks
    /// @{
    virtual void OnRenderBegin(const CommonFrameInfo& frameInfo) = 0;
    virtual void OnRenderEnd(const CommonFrameInfo& frameInfo) = 0;
    /// @}

    RenderDevice* renderDevice_{};
    bool bufferIsReady_{};
};

/// Writable and readable render buffer texture (2D or cubemap).
class URHO3D_API TextureRenderBuffer : public RenderBuffer
{
    URHO3D_OBJECT(TextureRenderBuffer, RenderBuffer);

public:
    TextureRenderBuffer(RenderPipelineInterface* renderPipeline,
        const RenderBufferParams& params, const Vector2& size = Vector2::ONE);
    ~TextureRenderBuffer() override;

    /// RenderBuffer implementation
    /// @{
    RawTexture* GetTexture() const override;
    RenderTargetView GetView(unsigned slice) const override;
    RenderTargetView GetReadOnlyDepthView(unsigned slice) const override;
    IntRect GetViewportRect() const override;
    /// @}

private:
    void OnRenderBegin(const CommonFrameInfo& frameInfo) override;
    void OnRenderEnd(const CommonFrameInfo& frameInfo) override;

    /// Immutable properties
    /// @{
    RenderBufferParams params_;
    Vector2 sizeMultiplier_ = Vector2::ONE;
    IntVector2 fixedSize_;
    /// @}

    /// Current frame info
    /// @{
    IntVector2 currentSize_;
    RawTexture* currentTexture_{};
    /// @}
};

/// Write-only viewport color render buffer.
class URHO3D_API ViewportColorRenderBuffer : public RenderBuffer
{
    URHO3D_OBJECT(ViewportColorRenderBuffer, RenderBuffer);

public:
    /// Construct.
    ViewportColorRenderBuffer(RenderPipelineInterface* renderPipeline);

    /// RenderBuffer implementation
    /// @{
    RawTexture* GetTexture() const override;
    RenderTargetView GetView(unsigned slice) const override;
    RenderTargetView GetReadOnlyDepthView(unsigned slice) const override;
    IntRect GetViewportRect() const override { return CheckIfBufferIsReady() ? viewportRect_ : IntRect::ZERO; }
    /// @}

private:
    void OnRenderBegin(const CommonFrameInfo& frameInfo) override;
    void OnRenderEnd(const CommonFrameInfo& frameInfo) override;

    IntRect viewportRect_{};
    /// Null if rendering to backbuffer.
    RenderSurface* renderTarget_{};
};

/// Write-only viewport depth-stenil texture.
class URHO3D_API ViewportDepthStencilRenderBuffer : public RenderBuffer
{
    URHO3D_OBJECT(ViewportDepthStencilRenderBuffer, RenderBuffer);

public:
    /// Construct.
    ViewportDepthStencilRenderBuffer(RenderPipelineInterface* renderPipeline);

    /// RenderBuffer implementation
    /// @{
    RawTexture* GetTexture() const override;
    RenderTargetView GetView(unsigned slice) const override;
    RenderTargetView GetReadOnlyDepthView(unsigned slice) const override;
    IntRect GetViewportRect() const override { return CheckIfBufferIsReady() ? viewportRect_ : IntRect::ZERO; }
    /// @}

private:
    void OnRenderBegin(const CommonFrameInfo& frameInfo) override;
    void OnRenderEnd(const CommonFrameInfo& frameInfo) override;

    IntRect viewportRect_{};
    /// Null if rendering to backbuffer or invalid.
    RenderSurface* depthStencil_{};
};

}
