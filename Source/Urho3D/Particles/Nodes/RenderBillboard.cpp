
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

#include "RenderBillboard.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "RenderBillboardInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void RenderBillboard::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<RenderBillboard>();
    URHO3D_ACCESSOR_ATTRIBUTE("Material", GetMaterial, SetMaterial, ResourceRef, ResourceRef{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rows", GetRows, SetRows, int, int{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Columns", GetColumns, SetColumns, int, int{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Face Camera Mode", GetFaceCameraMode, SetFaceCameraMode, int, int{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Sort By Distance", GetSortByDistance, SetSortByDistance, bool, bool{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Worldspace", GetIsWorldspace, SetIsWorldspace, bool, bool{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Crop", GetCrop, SetCrop, Rect, Rect::POSITIVE, AM_DEFAULT);
}


RenderBillboard::RenderBillboard(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "pos", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "size", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "frame", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "color", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "rotation", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "direction", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned RenderBillboard::EvaluateInstanceSize() const
{
    return sizeof(RenderBillboardInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* RenderBillboard::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    RenderBillboardInstance* instance = new (ptr) RenderBillboardInstance();
    instance->Init(this, layer);
    return instance;
}

void RenderBillboard::SetMaterial(ResourceRef value) { material_ = value; }

ResourceRef RenderBillboard::GetMaterial() const { return material_; }

void RenderBillboard::SetRows(int value) { rows_ = value; }

int RenderBillboard::GetRows() const { return rows_; }

void RenderBillboard::SetColumns(int value) { columns_ = value; }

int RenderBillboard::GetColumns() const { return columns_; }

void RenderBillboard::SetFaceCameraMode(int value) { faceCameraMode_ = value; }

int RenderBillboard::GetFaceCameraMode() const { return faceCameraMode_; }

void RenderBillboard::SetSortByDistance(bool value) { sortByDistance_ = value; }

bool RenderBillboard::GetSortByDistance() const { return sortByDistance_; }

void RenderBillboard::SetIsWorldspace(bool value) { isWorldspace_ = value; }

bool RenderBillboard::GetIsWorldspace() const { return isWorldspace_; }

void RenderBillboard::SetCrop(Rect value) { crop_ = value; }

Rect RenderBillboard::GetCrop() const { return crop_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
