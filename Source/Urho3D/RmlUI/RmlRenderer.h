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


#include "../Container/Ptr.h"

#include <RmlUi/Core/RenderInterface.h>


namespace Urho3D
{

class Context;
class IndexBuffer;
class VertexBuffer;
class Texture;

namespace Detail
{

struct CompiledGeometryForRml
{
	SharedPtr<VertexBuffer> vertexBuffer_;
	SharedPtr<IndexBuffer> indexBuffer_;
	SharedPtr<Texture> texture_;
};

class URHO3D_API RmlRenderer : public Object, public Rml::RenderInterface
{
    URHO3D_OBJECT(RmlRenderer, Object);
public:
    /// Construct.
    explicit RmlRenderer(Context* context);
    /// Compile geometry for later reuse.
    void CompileGeometry(CompiledGeometryForRml& compiledGeometryOut, Rml::Vertex* vertices, int numVertices, int* indices, int numIndices, const Rml::TextureHandle texture);
    /// Compile geometry for later reuse.
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int numVertices, int* indices, int numIndices, const Rml::TextureHandle texture) override;
    /// Render compiled geometry.
    void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometryHandle, const Rml::Vector2f& translation) override;
    /// Compile and render geometry.
	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;
    /// Free compiled geometry which was returned by previous call to CompileGeometry().
    void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;
    /// Enable or disable scissor region.
    void EnableScissorRegion(bool enable) override;
    /// Update scissor region.
    void SetScissorRegion(int x, int y, int width, int height) override;
    /// Apply requested scissor region.
    void ApplyScissorRegion(RenderSurface* surface, const IntVector2& viewSize);
    /// Load a texture from resource cache.
    bool LoadTexture(Rml::TextureHandle& textureOut, Rml::Vector2i& sizeOut, const Rml::String& source) override;
    /// Create a texture from pixelbuffer.
    bool GenerateTexture(Rml::TextureHandle& handleOut, const Rml::byte* source, const Rml::Vector2i& size) override;
    /// Release a texture that was previously created by LoadTexture() or GenerateTexture().
    void ReleaseTexture(Rml::TextureHandle textureHandle) override;
    /// Set or unset a custom transform.
    void SetTransform(const Rml::Matrix4f* transform) override;

private:
    /// Perform initialization tasks that require graphics subsystem.
    void InitializeGraphics();

    /// Graphics subsystem instance.
    WeakPtr<Graphics> graphics_;
    /// Set to true when RmlUi requests use of scissor region.
    bool scrissorEnabled_ = false;
    /// Set to true when RmlUi requests use of a specific
    bool transformEnabled_ = false;
    /// Scissor region set by RmlUi.
    IntRect scissor_;
    /// Temporary buffer used for rendering uncompiled geometry.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Temporary buffer used for rendering uncompiled geometry.
    SharedPtr<IndexBuffer> indexBuffer_;
    /// Transform requested by RmlUi. This pointer points either to data owned by RmlUi or to Matrix4::IDENTITY.data().
    const float* matrix_ = nullptr;
    /// Last viewport size.
    IntVector2 lastViewportSize_;
    /// Flag indicating last batch rendered into a custom rendertarget.
    bool lastViewportHadRendertarget_ = false;
    /// Last projection matrix. Reused when viewport size does not change.
    Matrix4 projection_;

    /// Pixel shader for rendering untextured elements.
    SharedPtr<ShaderVariation> noTexturePS_;
    /// Vertex shader for rendering untextured elements.
    SharedPtr<ShaderVariation> noTextureVS_;
    /// Pixel shader for rendering opaque textured elements.
    SharedPtr<ShaderVariation> textureOpaquePS_;
    /// Pixel shader for rendering transparent textured elements.
    SharedPtr<ShaderVariation> textureAlphaPS_;
    /// Vertex shader for rendering textured elements.
    SharedPtr<ShaderVariation> textureVS_;
    /// Pixel shader for applying scissor regions when custom transform matrix is in effect.
    SharedPtr<ShaderVariation> stencilPS_;
    /// Vertex shader for applying scissor regions when custom transform matrix is in effect.
    SharedPtr<ShaderVariation> stencilVS_;
};

}   // namespace Detail

}   // namespace Urho3D
