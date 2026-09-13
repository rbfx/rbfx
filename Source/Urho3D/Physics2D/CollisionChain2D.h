// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Physics2D/CollisionShape2D.h"

namespace Urho3D
{
/// 2D chain collision component.
class URHO3D_API CollisionChain2D : public CollisionShape2D
{
    URHO3D_OBJECT(CollisionChain2D, CollisionShape2D);

public:
    /// Construct.
    explicit CollisionChain2D(Context* context);
    /// Destruct.
    ~CollisionChain2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set loop.
    /// @property
    void SetLoop(bool loop);
    /// Set vertex count.
    /// @property
    void SetVertexCount(unsigned count);
    /// Set vertex.
    void SetVertex(unsigned index, const Vector2& vertex);
    /// Set vertices.
    void SetVertices(const ea::vector<Vector2>& vertices);
    /// Set vertices attribute.
    void SetVerticesAttr(const ea::vector<unsigned char>& value);

    /// Return loop.
    /// @property
    bool GetLoop() const { return loop_; }

    /// Return vertex count.
    /// @property
    unsigned GetVertexCount() const { return vertices_.size(); }

    /// Return vertex.
    const Vector2& GetVertex(unsigned index) const { return (index < vertices_.size()) ? vertices_[index] : Vector2::ZERO; }

    /// Return vertices.
    const ea::vector<Vector2>& GetVertices() const { return vertices_; }

    /// Return vertices attribute.
    ea::vector<unsigned char> GetVerticesAttr() const;

private:
    /// Apply node world scale.
    void ApplyNodeWorldScale() override;
    /// Recreate fixture.
    void RecreateFixture();

    /// Chain shape.
    b2ChainShape chainShape_;
    /// Loop.
    bool loop_;
    /// Vertices.
    ea::vector<Vector2> vertices_;
};

}
