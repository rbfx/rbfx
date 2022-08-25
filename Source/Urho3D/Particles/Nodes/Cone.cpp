
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

#include "Cone.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "ConeInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Cone::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Cone>();
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius Thickness", GetRadiusThickness, SetRadiusThickness, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Angle", GetAngle, SetAngle, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Translation", GetTranslation, SetTranslation, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("From", GetFrom, SetFrom, int, int{}, AM_DEFAULT);
}


Cone::Cone(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "position", ParticleGraphContainerType::Span),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "velocity", ParticleGraphContainerType::Span),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Cone::EvaluateInstanceSize() const
{
    return sizeof(ConeInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Cone::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    ConeInstance* instance = new (ptr) ConeInstance();
    instance->Init(this, layer);
    return instance;
}

void Cone::SetRadius(float value) { radius_ = value; }

float Cone::GetRadius() const { return radius_; }

void Cone::SetRadiusThickness(float value) { radiusThickness_ = value; }

float Cone::GetRadiusThickness() const { return radiusThickness_; }

void Cone::SetAngle(float value) { angle_ = value; }

float Cone::GetAngle() const { return angle_; }

void Cone::SetLength(float value) { length_ = value; }

float Cone::GetLength() const { return length_; }

void Cone::SetTranslation(Vector3 value) { translation_ = value; }

Vector3 Cone::GetTranslation() const { return translation_; }

void Cone::SetRotation(Quaternion value) { rotation_ = value; }

Quaternion Cone::GetRotation() const { return rotation_; }

void Cone::SetScale(Vector3 value) { scale_ = value; }

Vector3 Cone::GetScale() const { return scale_; }

void Cone::SetFrom(int value) { from_ = value; }

int Cone::GetFrom() const { return from_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
