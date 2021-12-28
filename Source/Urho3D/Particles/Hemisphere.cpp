//
// Copyright (c) 2021 the rbfx project.
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

#include "../Precompiled.h"

#include "Hemisphere.h"
#include "ParticleGraphSystem.h"

#include "Helpers.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
namespace
{
const char* emitFromNames[] = {"Base", "Volume", "Surface", "Edge", "Vertex", nullptr};
}

Hemisphere::Hemisphere(Context* context)
    : AbstractNodeType(context,
        PinArray{
            ParticleGraphPin(ParticleGraphPinFlag::MutableType, "position", PGCONTAINER_SPAN),
            ParticleGraphPin(ParticleGraphPinFlag::MutableType, "velocity", PGCONTAINER_SPAN),
        })
    , radius_(0.0f)
    , radiusThickness_(1.0f)
    , rotation_(Quaternion::IDENTITY)
    , position_(Vector3::ZERO)
    , scale_(Vector3::ONE)
    , emitFrom_(EmitFrom::Volume)
{
}

void Hemisphere::RegisterObject(ParticleGraphSystem* context)
{
    auto ref = context->AddReflection<Hemisphere>();
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius Thickness", GetRadiusThickness, SetRadiusThickness, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion::IDENTITY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3::ONE, AM_DEFAULT);
    ref->AddAttribute(Urho3D::AttributeInfo(Urho3D::VAR_STRING, "From",
        Urho3D::MakeVariantAttributeAccessor<Hemisphere>([](const Hemisphere& self, Urho3D::Variant& value)
            { value = emitFromNames[static_cast<int>(self.emitFrom_)]; },
            [](Hemisphere& self, const Urho3D::Variant& value)
            {
                const char** name = emitFromNames;
                const auto expectedName = value.Get<ea::string>();
                int index = 0;
                while (name[index])
                {
                    if (expectedName == name[index])
                    {
                        self.emitFrom_ = static_cast<EmitFrom>(index);
                        return;
                    }
                    ++index;
                }
                self.emitFrom_ = static_cast<EmitFrom>(0);
            }),
        emitFromNames, static_cast<int>(EmitFrom::Volume), AM_DEFAULT));
}

void Hemisphere::Generate(Vector3& pos, Vector3& vel) const
{
    Vector3 direction(Random(2.0f) - 1.0f, Random(2.0f) - 1.0f, Random(2.0f) - 1.0f);
    direction.Normalize();
    direction.z_ = Abs(direction.z_);

    float r = radius_;
    if (radiusThickness_ > 0.0f && emitFrom_ != EmitFrom::Surface)
    {
        r *= 1.0f - Random() * radiusThickness_;
    }
    switch (emitFrom_)
    {
    case EmitFrom::Base:
        vel = direction;
        pos = Vector3::ZERO;
        break;
    case EmitFrom::Surface:
        vel = direction;
        pos = direction * radius_;
        break;
    default:
        vel = direction;
        pos = direction * Pow(Random(), 1.0f / 3.0f) * 0.5f;
        break;
    }
}

Matrix3x4 Hemisphere::GetShapeTransform() const { return Matrix3x4(position_, rotation_, scale_); }
} // namespace ParticleGraphNodes
} // namespace Urho3D
