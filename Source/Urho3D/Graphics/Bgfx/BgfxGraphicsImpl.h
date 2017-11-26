//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../../Container/HashMap.h"
#include "../../Core/Timer.h"
#include "../../Graphics/Texture2D.h"

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>

namespace Urho3D
{

using ShaderProgramMap = HashMap<Pair<ShaderVariation*, ShaderVariation*>, SharedPtr<ShaderProgram> >;

/// Cached state of a frame buffer handle
struct FrameBufferHandle
{
    FrameBufferHandle()
    {
        handle_.idx = bgfx::kInvalidHandle;
        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
            colorAttachments_[i] = nullptr;
    }

    /// Frame buffer handle.
    bgfx::FrameBufferHandle handle_;
    /// Bound color attachment textures.
    RenderSurface* colorAttachments_[MAX_RENDERTARGETS];
    /// Bound depth/stencil attachment.
    RenderSurface* depthAttachment_;
};

/// %Graphics subsystem implementation. Holds API-specific objects.
class URHO3D_API GraphicsImpl
{
    friend class Graphics;

public:
    /// Construct.
    GraphicsImpl();
    /// Destruct.
    virtual ~GraphicsImpl();
    /// Get current view.
    uint8_t GetCurrentView() const { return view_; }
    /// Set current view.
    void SetCurrentView(const uint8_t view);

private:
    /// Backbuffer framebuffer.
    bgfx::FrameBufferHandle backbuffer_;
    /// List of framebuffers.
    //HashMap<unsigned long long, FrameBufferHandle> frameBuffers_;
    Vector<bgfx::FrameBufferHandle> frameBuffers_;
    /// Current framebuffer.
    bgfx::FrameBufferHandle currentFramebuffer_;
    /// Current view.
    uint8_t view_;
	/// Shader programs.
	ShaderProgramMap shaderPrograms_;
    /// Current shader program.
    ShaderProgram* shaderProgram_;
    /// Current depth of primitive.
    uint32_t drawDistance_;
    /// Rendertargets dirty flag.
    bool renderTargetsDirty_;
    /// Vertex declaration dirty flag.
    bool vertexDeclarationDirty_;
    /// Scissor rect dirty flag.
    bool scissorRectDirty_;
    /// Stencil ref dirty flag.
    bool stencilRefDirty_;
    /// BGFX state dirty flag.
    bool stateDirty_;
};

}
