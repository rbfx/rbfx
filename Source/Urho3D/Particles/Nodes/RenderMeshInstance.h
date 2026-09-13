// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
