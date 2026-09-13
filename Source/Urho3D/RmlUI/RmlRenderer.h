// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once


#include "../Container/Ptr.h"
#include "../RenderPipeline/BatchStateCache.h"

#include <RmlUi/Core/RenderInterface.h>


namespace Urho3D
{

class Context;
class DrawCommandQueue;
class DynamicIndexBuffer;
class DynamicVertexBuffer;
class Texture2D;

namespace Detail
{

class URHO3D_API RmlRenderer : public Object, public Rml::RenderInterface
{
    URHO3D_OBJECT(RmlRenderer, Object);

public:
    explicit RmlRenderer(Context* context);

    void BeginRendering();
    void EndRendering();

    /// Rml::RenderInterface implementation
    /// @{
    bool GenerateTexture(Rml::TextureHandle& handleOut, const Rml::byte* source, const Rml::Vector2i& size) override;
    void ReleaseTexture(Rml::TextureHandle textureHandle) override;
    bool LoadTexture(Rml::TextureHandle& textureOut, Rml::Vector2i& sizeOut, const Rml::String& source) override;

    void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;
    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(int x, int y, int width, int height) override;
    void SetTransform(const Rml::Matrix4f* transform) override;
    /// @}

private:
    /// Perform initialization tasks that require graphics subsystem.
    void InitializeGraphics();
    Material* GetBatchMaterial(Texture2D* texture);

    /// Default materials
    /// @{
    SharedPtr<Material> noTextureMaterial_;
    SharedPtr<Material> alphaMapMaterial_;
    SharedPtr<Material> diffMapMaterial_;
    /// @}

    /// Cached between frames and calls
    /// @{
    SharedPtr<DefaultUIBatchStateCache> batchStateCache_;
    SharedPtr<DynamicVertexBuffer> vertexBuffer_;
    SharedPtr<DynamicIndexBuffer> indexBuffer_;
    /// @}

    /// Constant between BeginRendering/EndRendering
    /// @{
    UIBatchStateCreateContext batchStateCreateContext_;
    bool flipRect_{};
    bool isRenderSurfaceSRGB_{};
    IntVector2 viewportSize_{};
    DrawCommandQueue* drawQueue_{};
    ea::vector<SharedPtr<Texture2D>> textures_{};
    Matrix4 projection_;
    /// @}

    bool scissorEnabled_ = false;
    IntRect scissor_;

    bool transformEnabled_ = false;
    Matrix3x4 transform_;
};

}   // namespace Detail

}   // namespace Urho3D
