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
#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Resource/ResourceCache.h"
#include "../Math/Matrix4.h"
#include "../RmlUI/RmlRenderer.h"

namespace Urho3D
{

namespace Detail
{

RmlRenderer::RmlRenderer(Context* context)
    : Object(context)
    , vertexBuffer_(context->CreateObject<VertexBuffer>())
    , indexBuffer_(context->CreateObject<IndexBuffer>())
{
    InitializeGraphics();
    SubscribeToEvent(E_SCREENMODE, [this](StringHash, VariantMap&) { InitializeGraphics(); });
}

void RmlRenderer::InitializeGraphics()
{
    graphics_ = context_->GetSubsystem<Graphics>();

    if (graphics_.Null() || !graphics_->IsInitialized())
        return;

    stencilPS_ = graphics_->GetShader(PS, "Stencil");
    stencilVS_ = graphics_->GetShader(VS, "Stencil");
    noTexturePS_ = graphics_->GetShader(PS, "Basic", "VERTEXCOLOR");
    noTextureVS_ = graphics_->GetShader(VS, "Basic", "VERTEXCOLOR");
    textureVS_ = graphics_->GetShader(VS, "Basic", "DIFFMAP VERTEXCOLOR");
    textureOpaquePS_ = graphics_->GetShader(PS, "Basic", "DIFFMAP VERTEXCOLOR");
    textureAlphaPS_ = graphics_->GetShader(PS, "Basic", "ALPHAMAP VERTEXCOLOR");
}

void RmlRenderer::CompileGeometry(CompiledGeometryForRml& compiledGeometryOut, Rml::Vertex* vertices, int numVertices, int* indices, int numIndices, const Rml::TextureHandle texture)
{
    VertexBuffer* vertexBuffer;
    IndexBuffer* indexBuffer;
    compiledGeometryOut.texture_ = reinterpret_cast<Urho3D::Texture*>(texture);
    if (compiledGeometryOut.vertexBuffer_.Null())
        compiledGeometryOut.vertexBuffer_ = vertexBuffer = context_->CreateObject<VertexBuffer>().Detach();
    else
        vertexBuffer = compiledGeometryOut.vertexBuffer_.Get();

    if (compiledGeometryOut.indexBuffer_.Null())
        compiledGeometryOut.indexBuffer_ = indexBuffer = context_->CreateObject<IndexBuffer>().Detach();
    else
        indexBuffer = compiledGeometryOut.indexBuffer_.Get();

    vertexBuffer->SetSize(numVertices, MASK_POSITION | MASK_COLOR | (texture ? MASK_TEXCOORD1 : MASK_NONE), true);
    indexBuffer->SetSize(numIndices, true);

    float* vertexData = static_cast<float*>(vertexBuffer->Lock(0, numVertices, true));
    assert(vertexData != nullptr);
    for (int i = 0; i < numVertices; ++i)
    {
        *vertexData++ = vertices[i].position.x;
        *vertexData++ = vertices[i].position.y;
        *vertexData++ = 0.f;
        *((unsigned*)vertexData++) = (vertices[i].colour.alpha << 24u) | (vertices[i].colour.blue << 16u) | (vertices[i].colour.green << 8u) | vertices[i].colour.red;
        if (texture)
        {
            *vertexData++ = vertices[i].tex_coord.x;
            *vertexData++ = vertices[i].tex_coord.y;
        }
    }
    vertexBuffer->Unlock();
    indexBuffer->SetDataRange(indices, 0, numIndices);
}

Rml::CompiledGeometryHandle RmlRenderer::CompileGeometry(Rml::Vertex* vertices, int numVertices, int* indices, int numIndices, const Rml::TextureHandle texture)
{
    CompiledGeometryForRml* geom = new CompiledGeometryForRml();
    CompileGeometry(*geom, vertices, numVertices, indices, numIndices, texture);
    return reinterpret_cast<Rml::CompiledGeometryHandle>(geom);
}

void RmlRenderer::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometryHandle, const Rml::Vector2f& translation)
{
    CompiledGeometryForRml* geometry = reinterpret_cast<CompiledGeometryForRml*>(geometryHandle);

    // Engine does not render when window is closed or device is lost
    assert(graphics_ && graphics_->IsInitialized() && !graphics_->IsDeviceLost());

    RenderSurface* surface = graphics_->GetRenderTarget(0);
    IntVector2 viewSize = graphics_->GetViewport().Size();
    if (viewSize != lastViewportSize_ || lastViewportHadRendertarget_ != (surface != nullptr))
    {
        lastViewportSize_ = viewSize;
        lastViewportHadRendertarget_ = (surface != nullptr);

        Vector2 invScreenSize(1.0f / (float)viewSize.x_, 1.0f / (float)viewSize.y_);
        Vector2 scale(2.0f * invScreenSize.x_, -2.0f * invScreenSize.y_);
        Vector2 offset(-1.0f, 1.0f);
        if (surface)
        {
    #ifdef URHO3D_OPENGL
            // On OpenGL, flip the projection if rendering to a texture so that the texture can be addressed in the
            // same way as a render texture produced on Direct3D.
            offset.y_ = -offset.y_;
            scale.y_ = -scale.y_;
    #endif
        }

        const float farClip_ = 1000.0f;
        projection_ = Matrix4::IDENTITY;
        projection_.m00_ = scale.x_;
        projection_.m03_ = offset.x_;
        projection_.m11_ = scale.y_;
        projection_.m13_ = offset.y_;
        projection_.m22_ = 1.0f / farClip_;
        projection_.m23_ = 0.0f;
        projection_.m33_ = 1.0f;
    }

    graphics_->ClearParameterSources();
    graphics_->SetBlendMode(BLEND_ALPHA);
    graphics_->SetColorWrite(true);
    graphics_->SetCullMode(CULL_NONE);
    graphics_->SetDepthTest(CMP_ALWAYS);
    graphics_->SetDepthWrite(false);
    graphics_->SetFillMode(FILL_SOLID);
    graphics_->SetStencilTest(false);
    graphics_->SetTexture(0, nullptr);

    ApplyScissorRegion(surface, viewSize);

    ShaderVariation* ps;
    ShaderVariation* vs;

    if (geometry->texture_.Null())
    {
        ps = noTexturePS_;
        vs = noTextureVS_;
    }
    else
    {
        // If texture contains only an alpha channel, use alpha shader (for fonts)
        vs = textureVS_;
        if (geometry->texture_->GetFormat() == Graphics::GetAlphaFormat())
            ps = textureAlphaPS_;
        else
            ps = textureOpaquePS_;
    }

    graphics_->SetTexture(0, geometry->texture_);
    graphics_->SetVertexBuffer(geometry->vertexBuffer_);
    graphics_->SetIndexBuffer(geometry->indexBuffer_);
    graphics_->SetShaders(vs, ps);
    graphics_->SetLineAntiAlias(true);

    // Apply translation
    Matrix4 translate = Matrix4::IDENTITY;
    translate.SetTranslation(Vector3(translation.x, translation.y, 0.f));
    Matrix4 model = (matrix_ ? Matrix4(matrix_) : Matrix4::IDENTITY) * translate;

    if (graphics_->NeedParameterUpdate(SP_OBJECT, this))
        graphics_->SetShaderParameter(VSP_MODEL, model);
    if (graphics_->NeedParameterUpdate(SP_CAMERA, this))
        graphics_->SetShaderParameter(VSP_VIEWPROJ, projection_);
    if (graphics_->NeedParameterUpdate(SP_MATERIAL, this))
        graphics_->SetShaderParameter(PSP_MATDIFFCOLOR, Color(1.0f, 1.0f, 1.0f, 1.0f));

    float elapsedTime = context_->GetSubsystem<Time>()->GetElapsedTime();
    graphics_->SetShaderParameter(VSP_ELAPSEDTIME, elapsedTime);
    graphics_->SetShaderParameter(PSP_ELAPSEDTIME, elapsedTime);

    graphics_->Draw(TRIANGLE_LIST, 0, geometry->indexBuffer_->GetIndexCount(), 0, geometry->vertexBuffer_->GetVertexCount());
}

void RmlRenderer::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
    // Could this be optimized?
    CompiledGeometryForRml geometry;
    geometry.vertexBuffer_ = vertexBuffer_;
    geometry.indexBuffer_ = indexBuffer_;
    CompileGeometry(geometry, vertices, num_vertices, indices, num_indices, texture);
    RenderCompiledGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(&geometry), translation);
}

void RmlRenderer::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry)
{
    delete reinterpret_cast<CompiledGeometryForRml*>(geometry);
}

void RmlRenderer::EnableScissorRegion(bool enable)
{
    scrissorEnabled_ = enable;
}

void RmlRenderer::SetScissorRegion(int x, int y, int width, int height)
{
    scissor_.left_ = x;
    scissor_.top_ = y;
    scissor_.bottom_ = y + height;
    scissor_.right_ = x + width;
}

void RmlRenderer::ApplyScissorRegion(RenderSurface* surface, const IntVector2& viewSize)
{
    if (!scrissorEnabled_)
    {
        graphics_->SetScissorTest(false);
        return;
    }

    IntRect scissor = scissor_;
#ifdef URHO3D_OPENGL
    // Flip scissor vertically if using OpenGL texture rendering
    if (surface)
    {
        int top = scissor.top_;
        int bottom = scissor.bottom_;
        scissor.top_ = viewSize.y_ - bottom;
        scissor.bottom_ = viewSize.y_ - top;
    }
#endif

    if (transformEnabled_)
    {
        graphics_->SetColorWrite(false);

        // Do not test the current value in the stencil buffer
        // always accept any value on there for drawing
        graphics_->SetStencilTest(true,
            Urho3D::CompareMode::CMP_ALWAYS,
            // Make every test succeed
            Urho3D::StencilOp::OP_REF,
            Urho3D::StencilOp::OP_KEEP,
            Urho3D::StencilOp::OP_KEEP, 1);

        Urho3D::VertexBuffer vertices(context_);
        vertices.SetSize(4, Urho3D::MASK_POSITION, true);

        float* vBuf = (float*)vertices.Lock(0, 4, true);
        // Vertex 1
        vBuf[0] = static_cast<float>(scissor.left_);
        vBuf[1] = static_cast<float>(scissor.top_);
        vBuf[2] = 0.f;
        // Vertex 2
        vBuf[3] = static_cast<float>(scissor.left_);
        vBuf[4] = static_cast<float>(scissor.bottom_);
        vBuf[5] = 0.f;
        // Vertex 3
        vBuf[6] = static_cast<float>(scissor.right_);
        vBuf[7] = static_cast<float>(scissor.bottom_);
        vBuf[8] = 0.f;
        // Vertex 4
        vBuf[9] = static_cast<float>(scissor.right_);
        vBuf[10] = static_cast<float>(scissor.top_);
        vBuf[11] = 0.f;
        vertices.Unlock();

        Urho3D::IndexBuffer indices(context_);
        indices.SetSize(4, true);

        int* iBuf = (int*)indices.Lock(0, 4, true);
        iBuf[0] = 1;
        iBuf[1] = 2;
        iBuf[2] = 0;
        iBuf[3] = 3;
        indices.Unlock();

        graphics_->SetVertexBuffer(&vertices);
        graphics_->SetIndexBuffer(&indices);
        graphics_->SetShaders(stencilVS_, stencilPS_);

        if (graphics_->NeedParameterUpdate(Urho3D::SP_OBJECT, this))
            graphics_->SetShaderParameter(Urho3D::VSP_MODEL, Urho3D::Matrix4::IDENTITY);
        if (graphics_->NeedParameterUpdate(Urho3D::SP_CAMERA, this))
            graphics_->SetShaderParameter(Urho3D::VSP_VIEWPROJ, projection_);

        graphics_->Draw(Urho3D::TRIANGLE_STRIP, 0, 4, 0, 4);

        graphics_->SetColorWrite(true);

        // Now we will only draw pixels where the corresponding stencil buffer value equals 1
        graphics_->SetStencilTest(true,
            Urho3D::CompareMode::CMP_EQUAL,
            // Make sure you will no longer (over)write stencil values, even if any test succeeds
            Urho3D::StencilOp::OP_KEEP,
            Urho3D::StencilOp::OP_KEEP,
            Urho3D::StencilOp::OP_KEEP, 1);
    }
    else
        graphics_->SetScissorTest(true, scissor);
}

bool RmlRenderer::LoadTexture(Rml::TextureHandle& textureOut, Rml::Vector2i& sizeOut, const Rml::String& source)
{
    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
    Texture2D* texture = cache->GetResource<Texture2D>(source.c_str());
    if (texture)
    {
        sizeOut.x = texture->GetWidth();
        sizeOut.y = texture->GetHeight();
        texture->AddRef();
    }
    textureOut = reinterpret_cast<Rml::TextureHandle>(texture);
    return true;
}

bool RmlRenderer::GenerateTexture(Rml::TextureHandle& handleOut, const Rml::byte* source, const Rml::Vector2i& size)
{
    Image image(context_);
    image.SetSize(size.x, size.y, 4);
    image.SetData(source);
    Texture2D* texture = context_->CreateObject<Texture2D>().Detach();
    texture->AddRef();
    texture->SetData(&image, true);
    handleOut = reinterpret_cast<Rml::TextureHandle>(texture);
    return true;
}

void RmlRenderer::ReleaseTexture(Rml::TextureHandle textureHandle)
{
    if (auto* texture = reinterpret_cast<Urho3D::Texture*>(textureHandle))
        texture->ReleaseRef();
}

void RmlRenderer::SetTransform(const Rml::Matrix4f* transform)
{
    transformEnabled_ = transform != nullptr;
    matrix_ = transform ? transform->data() : Matrix4::IDENTITY.Data();
}

}   // namespace Detail

}   // namespace Urho3D
