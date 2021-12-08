//
// Copyright (c) 2021 the rbfx project.
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

#include "Helpers.h"
#include "Urho3D/Graphics/BillboardSet.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

/// Render billboard
class URHO3D_API RenderBillboard : public AbstractNode<RenderBillboard, Vector3, Vector2>
{
    URHO3D_OBJECT(RenderBillboard, ParticleGraphNode)
public:
    /// Construct.
    explicit RenderBillboard(Context* context);

    class Instance : public AbstractNodeType::Instance
    {
    public:
        Instance(RenderBillboard* node, ParticleGraphLayerInstance* layer);
        ~Instance() override;
        void Prepare(unsigned numParticles);
        void UpdateParticle(unsigned index, const Vector3& pos, const Vector2& size);
        void Commit();

    protected:
        RenderBillboard* node_;
        SharedPtr<Urho3D::Node> sceneNode_;
        SharedPtr<Urho3D::BillboardSet> billboardSet_;
        SharedPtr<Urho3D::Octree> octree_;
    };

    template <typename Tuple>
    static void Op(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        instance->Prepare(numParticles);
        auto& pin0 = ea::get<0>(spans);
        auto& pin1 = ea::get<1>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            instance->UpdateParticle(i, pin0[i], pin1[i]);
        }
        instance->Commit();
    }

public:

    /// Return material.
    /// @property
    Material* GetMaterial() const;

    /// Set material.
    /// @property
    void SetMaterial(Material* material);

protected:
    /// Is the billboards attached to the node or rendered in a absolute world space coordinates.
    bool isWorldSpace_;
    /// Selected material.
    SharedPtr<Material> material_;
    /// Reference to material.
    ResourceRef materialRef_;

    //friend class AbstractNodeType;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
