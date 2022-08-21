
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

#include "Hemisphere.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "HemisphereInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Hemisphere::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Hemisphere>();
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius Thickness", GetRadiusThickness, SetRadiusThickness, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Translation", GetTranslation, SetTranslation, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("From", GetFrom, SetFrom, int, int{}, AM_DEFAULT);
}


Hemisphere::Hemisphere(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "position", ParticleGraphContainerType::Span),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "velocity", ParticleGraphContainerType::Span),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Hemisphere::EvaluateInstanceSize() const
{
    return sizeof(HemisphereInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Hemisphere::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    HemisphereInstance* instance = new (ptr) HemisphereInstance();
    instance->Init(this, layer);
    return instance;
}

void Hemisphere::SetRadius(float value) { radius_ = value; }

float Hemisphere::GetRadius() const { return radius_; }

void Hemisphere::SetRadiusThickness(float value) { radiusThickness_ = value; }

float Hemisphere::GetRadiusThickness() const { return radiusThickness_; }

void Hemisphere::SetTranslation(Vector3 value) { translation_ = value; }

Vector3 Hemisphere::GetTranslation() const { return translation_; }

void Hemisphere::SetRotation(Quaternion value) { rotation_ = value; }

Quaternion Hemisphere::GetRotation() const { return rotation_; }

void Hemisphere::SetScale(Vector3 value) { scale_ = value; }

Vector3 Hemisphere::GetScale() const { return scale_; }

void Hemisphere::SetFrom(int value) { from_ = value; }

int Hemisphere::GetFrom() const { return from_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
