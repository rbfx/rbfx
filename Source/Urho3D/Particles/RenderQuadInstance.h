//
// Copyright (c) 2021-2022 the rbfx project.
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

#include "RenderQuad.h"
#include "../Graphics/Octree.h"
#include "../Scene/Node.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/VertexBuffer.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class RenderQuadDrawable : public Drawable
{
    URHO3D_OBJECT(RenderQuadDrawable, Drawable);

public:
    /// Construct.
    explicit RenderQuadDrawable(Context* context);
    /// Destruct.
    ~RenderQuadDrawable() override;

    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly
    /// re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update).
    void UpdateGeometry(const FrameInfo& frame) override;
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    UpdateGeometryType GetUpdateGeometryType() override;

    /// Set material.
    /// @property
    void SetMaterial(Material* material);
    /// Mark for bounding box and vertex buffer update. Call after modifying the billboards.
    void Commit();

    /// Return material.
    /// @property
    Material* GetMaterial() const;

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
    /// Mark billboard vertex buffer to need an update.
    void MarkPositionsDirty();
private:
    /// Resize billboard vertex and index buffers.
    void UpdateBufferSize();
    /// Rewrite billboard vertex buffer.
    void UpdateVertexBuffer(const FrameInfo& frame);

    /// Geometry.
    SharedPtr<Geometry> geometry_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Index buffer.
    SharedPtr<IndexBuffer> indexBuffer_;
    /// Transform matrices for position and billboard orientation.
    Matrix3x4 transforms_[1];
};

class RenderQuadInstance final : public RenderQuad::InstanceBase
{
public:
    void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer) override;

    ~RenderQuadInstance() override;

    void Prepare(unsigned numParticles);

    template <typename T> void operator()(UpdateContext& context, unsigned numParticles, T transforms)
    {
        Prepare(numParticles);
        //auto* graphNode = static_cast<RenderQuad*>(GetGraphNode());
        //if (graphNode->GetIsWorldspace())
        //{
        //    for (unsigned i = 0; i < numParticles; ++i)
        //    {
        //        dst[i] = transforms[i];
        //    }
        //}
        //else
        //{
        //    auto localToWorld = GetNode()->GetWorldTransform();
        //    for (unsigned i = 0; i < numParticles; ++i)
        //    {
        //        dst[i] = localToWorld * transforms[i];
        //    }
        //}
    }

protected:
    SharedPtr<Urho3D::Node> sceneNode_;
    SharedPtr<RenderQuadDrawable> drawable_{};
    SharedPtr<Urho3D::Octree> octree_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
