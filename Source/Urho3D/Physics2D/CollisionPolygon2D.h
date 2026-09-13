// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Physics2D/CollisionShape2D.h"

namespace Urho3D
{

/// 2D polygon collision component.
class URHO3D_API CollisionPolygon2D : public CollisionShape2D
{
    URHO3D_OBJECT(CollisionPolygon2D, CollisionShape2D);

public:
    /// Construct.
    explicit CollisionPolygon2D(Context* context);
    /// Destruct.
    ~CollisionPolygon2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set vertex count.
    /// @property
    void SetVertexCount(unsigned count);
    /// Set vertex.
    void SetVertex(unsigned index, const Vector2& vertex);
    /// Set vertices.
    void SetVertices(const ea::vector<Vector2>& vertices);

    /// Return vertex count.
    /// @property
    unsigned GetVertexCount() const { return vertices_.size(); }

    /// Return vertex.
    const Vector2& GetVertex(unsigned index) const { return (index < vertices_.size()) ? vertices_[index] : Vector2::ZERO; }

    /// Return vertices.
    const ea::vector<Vector2>& GetVertices() const { return vertices_; }

    /// Set vertices attribute.
    void SetVerticesAttr(const ea::vector<unsigned char>& value);
    /// Return vertices attribute.
    ea::vector<unsigned char> GetVerticesAttr() const;

private:
    /// Apply node world scale.
    void ApplyNodeWorldScale() override;
    /// Recreate fixture.
    void RecreateFixture();

    /// Polygon shape.
    b2PolygonShape polygonShape_;
    /// Vertices.
    ea::vector<Vector2> vertices_;
};

}
