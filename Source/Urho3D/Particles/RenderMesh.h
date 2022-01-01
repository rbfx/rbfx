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
class Model;
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class RenderMeshDrawable;

/// Render static model mesh.
class URHO3D_API RenderMesh : public AbstractNode<RenderMesh, Matrix3x4>
{
    URHO3D_OBJECT(RenderMesh, ParticleGraphNode)
public:
    /// Construct.
    explicit RenderMesh(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    class Instance : public AbstractNodeType::Instance
    {
    public:
        Instance(RenderMesh* node, ParticleGraphLayerInstance* layer);
        ~Instance() override;
        ea::vector<Matrix3x4>& Prepare(unsigned numParticles);
        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& dst = Prepare(numParticles);
            auto& transforms = ea::get<0>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                dst[i] = transforms[i];
            }
        }
    protected:
        SharedPtr<Urho3D::Node> sceneNode_;
        SharedPtr<RenderMeshDrawable> drawable_ {};
        SharedPtr<Urho3D::Octree> octree_;

    };

public:
    /// Set model attribute.
    void SetModelAttr(const ResourceRef& value);
    /// Set materials attribute.
    void SetMaterialsAttr(const ResourceRefList& value);
    /// Return model attribute.
    ResourceRef GetModelAttr() const;
    /// Return materials attribute.
    const ResourceRefList& GetMaterialsAttr() const;

    /// Set model.
    /// @manualbind
    virtual void SetModel(Model* model);
    /// Set material on all geometries.
    /// @property
    virtual void SetMaterial(Material* material);
    /// Set material on one geometry. Return true if successful.
    /// @property{set_materials}
    virtual bool SetMaterial(unsigned index, Material* material);
    /// Return model.
    /// @property
    Model* GetModel() const { return model_; }
    /// Return material from the first geometry, assuming all the geometries use the same material.
    /// @property
    virtual Material* GetMaterial() const { return GetMaterial(0); }
    /// Return material by geometry index.
    /// @property{get_materials}
    virtual Material* GetMaterial(unsigned index) const;

protected:
    /// Is the billboards attached to the node or rendered in a absolute world space coordinates.
    bool isWorldSpace_;
    /// Model.
    SharedPtr<Model> model_;
    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
