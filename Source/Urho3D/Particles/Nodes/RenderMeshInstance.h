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

#include "RenderMesh.h"
#include "../../Graphics/StaticModel.h"
#include "../../Scene/Node.h"
#include "../../Graphics/Octree.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class RenderMeshDrawable : public StaticModel
{
    URHO3D_OBJECT(RenderMeshDrawable, StaticModel);

public:
    /// Construct.
    explicit RenderMeshDrawable(Context* context);

    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly
    /// re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;

    ea::vector<Matrix3x4> transforms_;
};


class RenderMeshInstance final : public RenderMesh::InstanceBase
{
public:
    void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer) override;

    void OnSceneSet(Scene* scene) override;
    void UpdateDrawableAttributes() override;

    ~RenderMeshInstance() override;

    ea::vector<Matrix3x4>& Prepare(unsigned numParticles);

    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Matrix3x4>& transforms)
    {
        auto& dst = Prepare(numParticles);
        auto* graphNode = static_cast<RenderMesh*>(GetGraphNode());
        if (graphNode->GetIsWorldspace())
        {
            for (unsigned i = 0; i < numParticles; ++i)
            {
                dst[i] = transforms[i];
            }
        }
        else
        {
            auto localToWorld = GetNode()->GetWorldTransform();
            for (unsigned i = 0; i < numParticles; ++i)
            {
                dst[i] = localToWorld * transforms[i];
            }
        }
    }

protected:
    SharedPtr<Urho3D::Node> sceneNode_;
    SharedPtr<RenderMeshDrawable> drawable_{};
    SharedPtr<Urho3D::Octree> octree_;
};


} // namespace ParticleGraphNodes

} // namespace Urho3D
