// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/LogicComponent.h"
namespace Urho3D
{

class URHO3D_API MoveAndOrbitComponent : public LogicComponent
{
    URHO3D_OBJECT(MoveAndOrbitComponent, LogicComponent);

public:
    /// Construct.
    explicit MoveAndOrbitComponent(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    /// Set movement velocity in node's local space.
    virtual void SetVelocity(const Vector3& velocity);
    /// Set yaw angle in degrees.
    virtual void SetYaw(float yaw);
    /// Set pitch angle in degrees.
    virtual void SetPitch(float pitch);
    /// Set distance limits.
    void SetDistanceLimits(float minDistance, float maxDistance);

    /// Get movement velocity in node's local space.
    const Vector3& GetVelocity() const { return velocity_; }
    /// Get yaw angle in degrees.
    float GetYaw() const { return yaw_; }
    /// Get pitch angle in degrees.
    float GetPitch() const { return pitch_; }
    /// Get min distance.
    float GetMinDistance() const { return minDistance_; }
    /// Get max distance.
    float GetMaxDistance() const { return maxDistance_; }

    /// Get yaw and pitch rotation.
    Quaternion GetYawPitchRotation() const { return Quaternion(pitch_, yaw_, 0.0f); }

private:
    Vector3 velocity_{};
    float yaw_{};
    float pitch_{};
    float minDistance_{0.5f};
    float maxDistance_{100.0f};
};

} // namespace Urho3D
