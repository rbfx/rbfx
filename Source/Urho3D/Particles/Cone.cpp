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

#include "ParticleGraphSystem.h"
#include "Cone.h"

#include "Helpers.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{

Cone::Cone(Context* context)
    : AbstractNodeType(context, PinArray{ParticleGraphPin(PGPIN_TYPE_MUTABLE, "out", PGCONTAINER_SPAN)})
    , radius_(0.0f)
    , angle_(45.0f)
    , rotation_(Quaternion::IDENTITY)
    , translation_(Vector3::ZERO)
{
}

void Cone::RegisterObject(ParticleGraphSystem* context)
{
    context->RegisterParticleGraphNodeFactory<Cone>();

    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Angle", GetAngle, SetAngle, float, 45.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion::IDENTITY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Translation", GetTranslation, SetTranslation, Vector3, Vector3::ZERO, AM_DEFAULT);
}

Vector3 Cone::Generate() const
{
    const float angle = Random(360.0f);
    const float radius = Sqrt(Random()) * Sin( Min(Max(angle_, 0.0f), 89.999f));
    const float height = Sqrt(1.0f - radius * radius);

    return Vector3(Cos(angle) * radius, Sin(angle) * radius, height);
}

Matrix3x4 Cone::GetShapeTransform() const
{
    return Matrix3x4(translation_, rotation_, 1.0f);
}
} // namespace ParticleGraphNodes
} // namespace Urho3D
