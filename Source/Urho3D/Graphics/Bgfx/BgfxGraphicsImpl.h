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
#include "../../Graphics/ShaderProgram.h"
#include "../../IO/Log.h"

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <bx/bx.h>

namespace Urho3D
{

struct BgfxCallback : public bgfx::CallbackI
{
virtual ~BgfxCallback()
{
}

virtual void fatal(bgfx::Fatal::Enum _code, const char* _str) override
{
    // Something unexpected happened, inform user and bail out.
    //URHO3D_LOGDEBUGF("Fatal error: 0x%08x: %s", _code, _str);
    URHO3D_LOGDEBUG("BGFX: Fatal error");

    // Must terminate, continuing will cause crash anyway.
    abort();
}

virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override
{
    URHO3D_LOGERROR("%s (%d): ", _filePath, _line);
    URHO3D_LOGERROR(_format, _argList);
}

virtual void profilerBegin(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override
{
}

virtual void profilerBeginLiteral(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override
{
}

virtual void profilerEnd() override
{
}

virtual uint32_t cacheReadSize(uint64_t _id) override
{
    return 0;
}

virtual bool cacheRead(uint64_t _id, void* _data, uint32_t _size) override
{
    return false;
}

virtual void cacheWrite(uint64_t _id, const void* _data, uint32_t _size) override
{
}

virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t /*_size*/, bool _yflip) override
{
}

virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t /*_pitch*/, bgfx::TextureFormat::Enum /*_format*/, bool _yflip) override
{
}

virtual void captureEnd() override
{
}

virtual void captureFrame(const void* _data, uint32_t /*_size*/) override
{
}
};

using ShaderProgramMap = HashMap<Pair<ShaderVariation*, ShaderVariation*>, SharedPtr<ShaderProgram> >;

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
    /// Set draw distance.
    void SetDrawDistance(const uint32_t drawDistance);
    /// Set instance vertex buffer.
    void SetInstanceBuffer(VertexBuffer* instanceBuffer);

private:
    /// Backbuffer framebuffer.
    bgfx::FrameBufferHandle backbuffer_;
    /// List of framebuffers.
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
    /// Primitive type.
    uint64_t primitiveType_;
    /// Current index buffer.
    bgfx::IndexBufferHandle indexBuffer_;
    /// Current dynamic index buffer.
    bgfx::DynamicIndexBufferHandle dynamicIndexBuffer_;
    /// Current vertex buffer.
    bgfx::VertexBufferHandle vertexBuffer_[MAX_VERTEX_STREAMS];
    /// Current dynamic vertex buffer.
    bgfx::DynamicVertexBufferHandle dynamicVertexBuffer_[MAX_VERTEX_STREAMS];
    /// Instance vertex buffer.
    VertexBuffer* instanceBuffer_;
    /// Instance offset.
    unsigned instanceOffset_;
    /// BGFX callback.
    BgfxCallback callback_;
};

}
