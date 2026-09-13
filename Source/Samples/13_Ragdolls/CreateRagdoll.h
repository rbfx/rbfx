// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>

using namespace Urho3D;

/// Custom component that creates a ragdoll upon collision.
class CreateRagdoll : public Component
{
    URHO3D_OBJECT(CreateRagdoll, Component);

public:
    /// Construct.
    explicit CreateRagdoll(Context* context);

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;

private:
    /// Handle scene node's physics collision.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
    /// Make a bone physical by adding RigidBody and CollisionShape components.
    void CreateRagdollBone(const ea::string& boneName, ShapeType type, const Vector3& size, const Vector3& position, const Quaternion& rotation);
    /// Join two bones with a Constraint component.
    void CreateRagdollConstraint(const ea::string& boneName, const ea::string& parentName, ConstraintType type, const Vector3& axis, const Vector3& parentAxis, const Vector2& highLimit, const Vector2& lowLimit, bool disableCollision = true);
};
