
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "RenderMesh.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "RenderMeshInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void RenderMesh::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<RenderMesh>();
    URHO3D_ACCESSOR_ATTRIBUTE("Model", GetModel, SetModel, ResourceRef, ResourceRef{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Material", GetMaterial, SetMaterial, ResourceRefList, ResourceRefList{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Worldspace", GetIsWorldspace, SetIsWorldspace, bool, bool{}, AM_DEFAULT);
}


RenderMesh::RenderMesh(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "transform", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned RenderMesh::EvaluateInstanceSize() const
{
    return sizeof(RenderMeshInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* RenderMesh::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    RenderMeshInstance* instance = new (ptr) RenderMeshInstance();
    instance->Init(this, layer);
    return instance;
}

void RenderMesh::SetModel(ResourceRef value) { model_ = value; }

ResourceRef RenderMesh::GetModel() const { return model_; }

void RenderMesh::SetMaterial(ResourceRefList value) { material_ = value; }

ResourceRefList RenderMesh::GetMaterial() const { return material_; }

void RenderMesh::SetIsWorldspace(bool value) { isWorldspace_ = value; }

bool RenderMesh::GetIsWorldspace() const { return isWorldspace_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
