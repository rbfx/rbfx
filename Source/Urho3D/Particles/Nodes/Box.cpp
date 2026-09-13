// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Box.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "BoxInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Box::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Box>();
    URHO3D_ACCESSOR_ATTRIBUTE("Box Thickness", GetBoxThickness, SetBoxThickness, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Translation", GetTranslation, SetTranslation, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("From", GetFrom, SetFrom, int, int{}, AM_DEFAULT);
}


Box::Box(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "position", ParticleGraphContainerType::Span),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "velocity", ParticleGraphContainerType::Span),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Box::EvaluateInstanceSize() const
{
    return sizeof(BoxInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Box::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    BoxInstance* instance = new (ptr) BoxInstance();
    instance->Init(this, layer);
    return instance;
}

void Box::SetBoxThickness(Vector3 value) { boxThickness_ = value; }

Vector3 Box::GetBoxThickness() const { return boxThickness_; }

void Box::SetTranslation(Vector3 value) { translation_ = value; }

Vector3 Box::GetTranslation() const { return translation_; }

void Box::SetRotation(Quaternion value) { rotation_ = value; }

Quaternion Box::GetRotation() const { return rotation_; }

void Box::SetScale(Vector3 value) { scale_ = value; }

Vector3 Box::GetScale() const { return scale_; }

void Box::SetFrom(int value) { from_ = value; }

int Box::GetFrom() const { return from_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
