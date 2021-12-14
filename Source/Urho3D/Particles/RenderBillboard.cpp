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

#include "RenderBillboard.h"
#include "ParticleGraphSystem.h"
#include "ParticleGraphLayerInstance.h"
#include "../Graphics/Material.h"
#include "../Graphics/Octree.h"
#include "../Scene/Scene.h"
#include "../IO/ArchiveSerialization.h"
#include "../Resource/ResourceCache.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
RenderBillboard::RenderBillboard(Context* context)
    : AbstractNodeType(context,
                       PinArray{
                           ParticleGraphPin(PGPIN_INPUT, "pos"),
                           ParticleGraphPin(PGPIN_INPUT, "size"),
                           ParticleGraphPin(PGPIN_INPUT, "frame"),
                           ParticleGraphPin(PGPIN_INPUT, "color"),
                       })
    , isWorldSpace_(false)
{
}

void RenderBillboard::RegisterObject(ParticleGraphSystem* context)
{
    context->RegisterParticleGraphNodeFactory<RenderBillboard>();

    URHO3D_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef,
                                    ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rows", GetRows, SetRows, unsigned, 1, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Columns", GetColumns, SetColumns, unsigned, 1, AM_DEFAULT);
}

RenderBillboard::Instance::Instance(RenderBillboard* node, ParticleGraphLayerInstance* layer)
    : AbstractNodeType::Instance(node, layer)
{
    const auto scene = GetScene();

    sceneNode_ = MakeShared<Node>(GetContext());

    billboardSet_ = sceneNode_->CreateComponent<BillboardSet>();
    billboardSet_->SetMaterial(node->material_);
    octree_ = scene->GetOrCreateComponent<Octree>();
    octree_->AddManualDrawable(billboardSet_);
}

RenderBillboard::Instance::~Instance() { octree_->RemoveManualDrawable(billboardSet_); }

void RenderBillboard::Instance::Prepare(unsigned numParticles)
{
    if (!GetGraphNodeInstace()->isWorldSpace_)
    {
         sceneNode_->SetWorldTransform(GetNode()->GetWorldTransform());
    }
    unsigned numBillboards = billboardSet_->GetNumBillboards();
    if (numBillboards < numParticles)
    {
        billboardSet_->SetNumBillboards(numParticles);
        numBillboards = numParticles;
    }
    auto& billboards = billboardSet_->GetBillboards();
    for (unsigned i = numParticles; i < numBillboards; ++i)
    {
        billboards[i].enabled_ = false;
    }
}

void RenderBillboard::Instance::UpdateParticle(unsigned index, const Vector3& pos, const Vector2& size, float frameIndex, Color& color)
{
    auto* billboard = billboardSet_->GetBillboard(index);
    billboard->enabled_ = true;
    billboard->position_ = pos;
    billboard->size_ = size;
    billboard->color_ = color;
    unsigned frame = frameIndex;
    unsigned x = frame % node_->columns_;
    unsigned y = (frame / node_->columns_) % node_->rows_;
    auto uvMin = Vector2(x, y) * node_->uvTileSize_;
    auto uvMax = uvMin + node_->uvTileSize_;
    billboard->uv_ = Rect(uvMin, uvMax);
}

void RenderBillboard::Instance::Commit() { billboardSet_->Commit(); }

Material* RenderBillboard::GetMaterial() const
{
    return context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialRef_.name_);
}

void RenderBillboard::SetMaterial(Material* material)
{
    material_ = material;
    if (material)
    {
        materialRef_ = ResourceRef(material->GetType(), material->GetName());
    }
    else
    {
        materialRef_ = ResourceRef();
    }
}

void RenderBillboard::SetMaterialAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
    materialRef_ = value;
}

ResourceRef RenderBillboard::GetMaterialAttr() const
{
    return materialRef_;
}

void RenderBillboard::SetRows(unsigned value)
{
    rows_ = ea::max(1u, value);
    UpdateUV();
}

unsigned RenderBillboard::GetRows() const
{
    return rows_;
}

void RenderBillboard::SetColumns(unsigned value)
{
    columns_ = ea::max(1u, value);
    UpdateUV();
}

unsigned RenderBillboard::GetColumns() const
{
    return columns_;
}

void RenderBillboard::UpdateUV()
{
    uvTileSize_ = Vector2(1.0f / columns_, 1.0f / rows_);
}
} // namespace ParticleGraphNodes

} // namespace Urho3D
