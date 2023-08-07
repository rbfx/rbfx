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
