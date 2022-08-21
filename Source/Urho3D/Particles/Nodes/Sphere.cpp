
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

#include "Sphere.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "SphereInstance.h"

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
