// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Physics2D/CollisionChain2D.h"
#include "../Physics2D/PhysicsUtils2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

CollisionChain2D::CollisionChain2D(Context* context) :
    CollisionShape2D(context),
    loop_(false)
{
    fixtureDef_.shape = &chainShape_;
}

CollisionChain2D::~CollisionChain2D() = default;

void CollisionChain2D::RegisterObject(Context* context)
{
    context->AddFactoryReflection<CollisionChain2D>(Category_Physics2D);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Loop", GetLoop, SetLoop, bool, false, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape2D);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Vertices", GetVerticesAttr, SetVerticesAttr, ea::vector<unsigned char>, Variant::emptyBuffer, AM_DEFAULT);
}

void CollisionChain2D::SetLoop(bool loop)
{
    if (loop == loop_)
        return;

    loop_ = loop;

    RecreateFixture();
}

void CollisionChain2D::SetVertexCount(unsigned count)
{
    vertices_.resize(count);
}

void CollisionChain2D::SetVertex(unsigned index, const Vector2& vertex)
{
    if (index >= vertices_.size())
        return;

    vertices_[index] = vertex;

    if (index == vertices_.size() - 1)
    {
        RecreateFixture();
    }
}

void CollisionChain2D::SetVertices(const ea::vector<Vector2>& vertices)
{
    vertices_ = vertices;

    RecreateFixture();
}

void CollisionChain2D::SetVerticesAttr(const ea::vector<unsigned char>& value)
{
    if (value.empty())
        return;

    ea::vector<Vector2> vertices;

    MemoryBuffer buffer(value);
    while (!buffer.IsEof())
        vertices.push_back(buffer.ReadVector2());

    SetVertices(vertices);
}

ea::vector<unsigned char> CollisionChain2D::GetVerticesAttr() const
{
    VectorBuffer ret;

    for (unsigned i = 0; i < vertices_.size(); ++i)
        ret.WriteVector2(vertices_[i]);

    return ret.GetBuffer();
}

void CollisionChain2D::ApplyNodeWorldScale()
{
    RecreateFixture();
}

void CollisionChain2D::RecreateFixture()
{
    ReleaseFixture();

    ea::vector<b2Vec2> b2Vertices;
    unsigned count = vertices_.size();
    b2Vertices.resize(count);

    Vector2 worldScale(cachedWorldScale_.x_, cachedWorldScale_.y_);
    for (unsigned i = 0; i < count; ++i)
        b2Vertices[i] = ToB2Vec2(vertices_[i] * worldScale);

    chainShape_.Clear();
    if (loop_)
        chainShape_.CreateLoop(&b2Vertices[0], count);
    else
        chainShape_.CreateChain(&b2Vertices[0], count);

    CreateFixture();
}

}
