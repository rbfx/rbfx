// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "RenderBillboard.h"
#include "../../Graphics/BillboardSet.h"
#include "../../Scene/Node.h"
#include "../../Graphics/Octree.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class RenderBillboardInstance final : public RenderBillboard::InstanceBase
{
public:
    ~RenderBillboardInstance() override;
    void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer) override;
    void OnSceneSet(Scene* scene) override;
    void UpdateDrawableAttributes() override;

    void Prepare(unsigned numParticles);
    void UpdateParticle(unsigned index, const Vector3& pos, const Vector2& size, float frameIndex, Color& color,
        float rotation, Vector3& direction);
    void Commit();

    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pin0,
        const SparseSpan<Vector2>& pin1, const SparseSpan<float>& frame, const SparseSpan<Color>& color,
        const SparseSpan<float>& rotation, const SparseSpan<Vector3>& direction)
    {
        Prepare(numParticles);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            UpdateParticle(i, pin0[i], pin1[i], frame[i], color[i], rotation[i], direction[i]);
        }
        Commit();
    }

protected:
    SharedPtr<Urho3D::Node> sceneNode_;
    SharedPtr<Urho3D::BillboardSet> billboardSet_;
    SharedPtr<Urho3D::Octree> octree_;
    unsigned cols_{};
    unsigned rows_{};
    Vector2 uvTileSize_;
    Vector2 cropOffset_;
    Vector2 cropSize_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
