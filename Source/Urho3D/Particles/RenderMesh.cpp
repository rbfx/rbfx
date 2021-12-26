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

#include "../Precompiled.h"

#include "../Graphics/Material.h"
#include "../Graphics/Octree.h"
#include "../Graphics/StaticModel.h"
#include "../IO/ArchiveSerialization.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "ParticleGraphLayerInstance.h"
#include "ParticleGraphSystem.h"
#include "RenderMesh.h"
#include "../Graphics/Model.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/ResourceEvents.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
class RenderMeshDrawable
    : public StaticModel
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


RenderMeshDrawable::RenderMeshDrawable(Context* context)
    : StaticModel(context)
{
}

void RenderMeshDrawable::UpdateBatches(const FrameInfo& frame)
{
    // Getting the world bounding box ensures the transforms are updated
    const BoundingBox& worldBoundingBox = GetWorldBoundingBox();
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    distance_ = frame.camera_->GetDistance(worldBoundingBox.Center());

    if (batches_.size() > 1)
    {
        for (unsigned i = 0; i < batches_.size(); ++i)
        {
            batches_[i].distance_ = frame.camera_->GetDistance(worldTransform * geometryData_[i].center_);
            batches_[i].worldTransform_ = transforms_.empty() ? &Matrix3x4::IDENTITY : &transforms_[0];
            batches_[i].numWorldTransforms_ = transforms_.size();
        }
    }
    else if (batches_.size() == 1)
    {
        batches_[0].distance_ = distance_;
        batches_[0].worldTransform_ = transforms_.empty() ? &Matrix3x4::IDENTITY : &transforms_[0];
        batches_[0].numWorldTransforms_ = transforms_.size();
    }

    float scale = worldBoundingBox.Size().DotProduct(DOT_SCALE);
    float newLodDistance = frame.camera_->GetLodDistance(distance_, scale, lodBias_);

    if (newLodDistance != lodDistance_)
    {
        lodDistance_ = newLodDistance;
        CalculateLodLevels();
    }
}


RenderMesh::RenderMesh(Context* context)
    : AbstractNodeType(context,
        PinArray{
            ParticleGraphPin(PGPIN_INPUT, "transform")
        })
    , isWorldSpace_(false)
    , materialsAttr_(Material::GetTypeStatic())
{
}

void RenderMesh::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<RenderMesh>();
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
        "Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Material", GetMaterialsAttr, SetMaterialsAttr, ResourceRefList,
        ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
}

RenderMesh::Instance::Instance(RenderMesh* node, ParticleGraphLayerInstance* layer)
    : AbstractNodeType::Instance(node, layer)
{
    const auto scene = GetScene();

    sceneNode_ = MakeShared<Node>(GetContext());
    drawable_ = MakeShared<RenderMeshDrawable>(GetContext());
    sceneNode_->AddComponent(drawable_, 0, LOCAL);
    drawable_->SetModel(node_->model_);
    drawable_->SetMaterialsAttr(node_->materialsAttr_);
    octree_ = scene->GetOrCreateComponent<Octree>();
    octree_->AddManualDrawable(drawable_);
}

RenderMesh::Instance::~Instance() { octree_->RemoveManualDrawable(drawable_); }

ea::vector<Matrix3x4>& RenderMesh::Instance::Prepare(unsigned numParticles)
{
    drawable_->transforms_.resize(numParticles);
    sceneNode_->SetWorldTransform(GetNode()->GetWorldTransform());
    //if (node_->material_ != drawable_->GetMaterial(0))
    //    drawable_->SetMaterial(node_->material_);
    return drawable_->transforms_;
}

void RenderMesh::SetModelAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto * model = cache->GetResource<Model>(value.name_);
    SetModel(model);
}

void RenderMesh::SetMaterialsAttr(const ResourceRefList& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    for (unsigned i = 0; i < value.names_.size() && i < materialsAttr_.names_.size(); ++i)
        SetMaterial(i, cache->GetResource<Material>(value.names_[i]));
}

ResourceRef RenderMesh::GetModelAttr() const { return GetResourceRef(model_, Model::GetTypeStatic()); }

const ResourceRefList& RenderMesh::GetMaterialsAttr() const
{
    return materialsAttr_;
}

void RenderMesh::SetModel(Model* model)
{
    if (model == model_)
        return;

    model_ = model;
    if (model_)
    {
        materialsAttr_.names_.resize(model_->GetNumGeometries());
    }
    else
    {
        materialsAttr_.names_.clear();
    }
}

void RenderMesh::SetMaterial(Material* material)
{
    for (unsigned i = 0; i < materialsAttr_.names_.size(); ++i)
        materialsAttr_.names_[i] = GetResourceName(material);
}

bool RenderMesh::SetMaterial(unsigned index, Material* material)
{
    if (index >= materialsAttr_.names_.size())
    {
        URHO3D_LOGERROR("Material index out of bounds");
        return false;
    }

    materialsAttr_.names_[index] = GetResourceName(material);
    return true;
}

Material* RenderMesh::GetMaterial(unsigned index) const
{
    return index < materialsAttr_.names_.size() ? GetSubsystem<ResourceCache>()->GetResource<Material>(materialsAttr_.names_[index]) : nullptr;
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
