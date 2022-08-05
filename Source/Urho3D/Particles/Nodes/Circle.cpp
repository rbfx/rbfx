
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
