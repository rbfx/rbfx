// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Sphere.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/SphereInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Sphere::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Sphere>();
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius Thickness", GetRadiusThickness, SetRadiusThickness, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Translation", GetTranslation, SetTranslation, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("From", GetFrom, SetFrom, int, int{}, AM_DEFAULT);
}


Sphere::Sphere(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "position", ParticleGraphContainerType::Span),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "velocity", ParticleGraphContainerType::Span),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Sphere::EvaluateInstanceSize() const
{
    return sizeof(SphereInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Sphere::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    SphereInstance* instance = new (ptr) SphereInstance();
    instance->Init(this, layer);
    return instance;
}

void Sphere::SetRadius(float value) { radius_ = value; }

float Sphere::GetRadius() const { return radius_; }

void Sphere::SetRadiusThickness(float value) { radiusThickness_ = value; }

float Sphere::GetRadiusThickness() const { return radiusThickness_; }

void Sphere::SetTranslation(Vector3 value) { translation_ = value; }

Vector3 Sphere::GetTranslation() const { return translation_; }

void Sphere::SetRotation(Quaternion value) { rotation_ = value; }

Quaternion Sphere::GetRotation() const { return rotation_; }

void Sphere::SetScale(Vector3 value) { scale_ = value; }

Vector3 Sphere::GetScale() const { return scale_; }

void Sphere::SetFrom(int value) { from_ = value; }

int Sphere::GetFrom() const { return from_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
