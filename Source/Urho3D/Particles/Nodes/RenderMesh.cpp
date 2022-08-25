
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
