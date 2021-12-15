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
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Render billboard
class URHO3D_API RenderBillboard : public AbstractNode<RenderBillboard, Vector3, Vector2, float, Color, float>
{
    URHO3D_OBJECT(RenderBillboard, ParticleGraphNode)
public:
    /// Construct.
    explicit RenderBillboard(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    class Instance : public AbstractNodeType::Instance
    {
    public:
        Instance(RenderBillboard* node, ParticleGraphLayerInstance* layer);
        ~Instance() override;
        void Prepare(unsigned numParticles);
        void UpdateParticle(unsigned index, const Vector3& pos, const Vector2& size, float frameIndex, Color& color,
                            float rotation);
        void Commit();

    protected:
        SharedPtr<Urho3D::Node> sceneNode_;
        SharedPtr<Urho3D::BillboardSet> billboardSet_;
        SharedPtr<Urho3D::Octree> octree_;
    };

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        instance->Prepare(numParticles);
        auto& pin0 = ea::get<0>(spans);
        auto& pin1 = ea::get<1>(spans);
        auto& frame = ea::get<2>(spans);
        auto& color = ea::get<3>(spans);
        auto& rotation = ea::get<4>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            instance->UpdateParticle(i, pin0[i], pin1[i], frame[i], color[i], rotation[i]);
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

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Set material attribute.
    ResourceRef GetMaterialAttr() const;

    /// Set number of rows in a sprite sheet.
    /// @property
    void SetRows(unsigned value);
    /// Get number of rows in a sprite sheet.
    /// @property 
    unsigned GetRows() const;

    /// Set number of columns in a sprite sheet.
    /// @property
    void SetColumns(unsigned value);
    /// Get number of columns in a sprite sheet.
    /// @property
    unsigned GetColumns() const;

protected:
    /// Update sprite sheet uv.
    void UpdateUV();

    /// Is the billboards attached to the node or rendered in a absolute world space coordinates.
    bool isWorldSpace_;
    /// Selected material.
    SharedPtr<Material> material_;
    /// Reference to material.
    ResourceRef materialRef_;
    /// Number of rows in a sprite sheet.
    unsigned rows_{1};
    /// Number of columns in a sprite sheet.
    unsigned columns_{1};
    /// Sprite sheet tile size.
    Vector2 uvTileSize_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
