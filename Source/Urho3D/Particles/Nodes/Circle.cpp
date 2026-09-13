// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Circle.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "CircleInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Circle::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Circle>();
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius Thickness", GetRadiusThickness, SetRadiusThickness, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Translation", GetTranslation, SetTranslation, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3{}, AM_DEFAULT);
}


Circle::Circle(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "position", ParticleGraphContainerType::Span),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "velocity", ParticleGraphContainerType::Span),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Circle::EvaluateInstanceSize() const
{
    return sizeof(CircleInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Circle::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    CircleInstance* instance = new (ptr) CircleInstance();
    instance->Init(this, layer);
    return instance;
}

void Circle::SetRadius(float value) { radius_ = value; }

float Circle::GetRadius() const { return radius_; }

void Circle::SetRadiusThickness(float value) { radiusThickness_ = value; }

float Circle::GetRadiusThickness() const { return radiusThickness_; }

void Circle::SetTranslation(Vector3 value) { translation_ = value; }

Vector3 Circle::GetTranslation() const { return translation_; }

void Circle::SetRotation(Quaternion value) { rotation_ = value; }

Quaternion Circle::GetRotation() const { return rotation_; }

void Circle::SetScale(Vector3 value) { scale_ = value; }

Vector3 Circle::GetScale() const { return scale_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
