//
// Copyright (c) 2008-2020 the Urho3D project.
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

#pragma once

#include "Urho3D/Scene/Component.h"
#include "Urho3D/Physics/RigidBody.h"

class btCapsuleShape;
class btPairCachingGhostObject;
class btKinematicCharacterController;

namespace Urho3D
{
class PhysicsWorld;
class DebugRenderer;

class URHO3D_API KinematicCharacterController : public Component
{
    URHO3D_OBJECT(KinematicCharacterController, Component);

public:
    /// Construct.
    KinematicCharacterController(Context* context);
    /// Destruct.
    ~KinematicCharacterController() override;

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);
    void OnSetAttribute(const AttributeInfo& attr, const Variant& src) override;

    /// Perform post-load after deserialization. Acquire the components from the scene nodes.
    void ApplyAttributes() override;
    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Return character position in world space without interpolation.
    Vector3 GetRawPosition() const;
    /// Adjust position of kinematic body.
    void AdjustRawPosition(const Vector3& offset, float smoothConstant);

    /// Set collision layer.
    void SetCollisionLayer(unsigned layer);
    /// Return collision layer.
    unsigned GetCollisionLayer() const { return colLayer_; }
    /// Set collision mask.
    void SetCollisionMask(unsigned mask);
    /// Return collision mask.
    unsigned GetCollisionMask() const { return colMask_; }
    /// Set collision group and mask.
    void SetCollisionLayerAndMask(unsigned layer, unsigned mask);

    /// Set rigid body trigger mode. In trigger mode kinematic characters don't affect rigid bodies and don't collide with each other.
    void SetTrigger(bool enable);
    /// Return whether this RigidBody is acting as a trigger.
    bool IsTrigger() const;

    /// Set gravity.
    void SetGravity(const Vector3 &gravity);
    /// Get gravity.
    const Vector3& GetGravity() const { return gravity_; }
    /// Set linear velocity damping factor.
    void SetLinearDamping(float linearDamping);
    /// Return linear velocity damping factor.
    float GetLinearDamping() const { return linearDamping_; }
    /// Set angular velocity damping factor.
    void SetAngularDamping(float angularDamping);
    /// Return linear velocity damping factor.
    float GetAngularDamping() const { return angularDamping_; }
    /// Set character height.
    void SetHeight(float height);
    /// Return character height.
    float GetHeight() const { return height_; }
    /// Set character diameter.
    void SetDiameter(float diameter);
    /// Return character diameter.
    float GetDiameter() const { return diameter_; }
    /// Set activate triggers flag.
    void SetActivateTriggers(bool activateTriggers);
    /// Get activate triggers flag.
    bool GetActivateTriggers() const { return activateTriggers_; }
    /// Set character collider offset.
    void SetOffset(const Vector3& offset);
    /// Return character collider offset.
    const Vector3& GetOffset() const { return colShapeOffset_; }
    /// Set step height.
    void SetStepHeight(float stepHeight);
    /// Return step height.
    float GetStepHeight() const { return stepHeight_; }
    /// Set max jump height.
    void SetMaxJumpHeight(float maxJumpHeight);
    /// Return max jump height.
    float GetMaxJumpHeight() const { return maxJumpHeight_; }
    /// Set fall speed (terminal velocity).
    void SetFallSpeed(float fallSpeed);
    /// Return fall speed (terminal velocity).
    float GetFallSpeed() const { return fallSpeed_; }
    /// Set jump speed.
    void SetJumpSpeed(float jumpSpeed);
    /// Return jump speed.
    float GetJumpSpeed() const { return jumpSpeed_; }
    /// Set max slope angle in degrees.
    void SetMaxSlope(float maxSlope);
    /// Return max slope angle in degrees.
    float GetMaxSlope() const { return maxSlope_; }
    /// Set walk increment. This is neither a direction nor a velocity, but the amount to increment the position each simulation iteration, regardless of dt.
    void SetWalkIncrement(const Vector3& walkDir);
    /// Check if character in on the ground.
    bool OnGround() const;
    /// Jump.
    void Jump(const Vector3 &jump = Vector3::ZERO);
    /// ApplyImpulse is same as Jump
    void ApplyImpulse(const Vector3 &impulse);
    /// Check if character can jump.
    bool CanJump() const;
    /// Set angular velocity.
    void SetAngularVelocity(const Vector3 &velocity);
    /// Return angular velocity.
    const Vector3 GetAngularVelocity() const;
    /// Set linear velocity.
    void SetLinearVelocity(const Vector3 &velocity);
    /// Return linear velocity.
    const Vector3 GetLinearVelocity() const;
    /// Draw debug geometry.
    virtual void DrawDebugGeometry();

protected:
    btCapsuleShape* GetOrCreateShape();
    void ResetShape();
    void ReleaseKinematic();
    void ApplySettings(bool readdToWorld);
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    void OnSceneSet(Scene* scene) override;
    void ActivateIfEnabled();
    void AddKinematicToWorld();
    void RemoveKinematicFromWorld();
    bool IsAddedToWorld() const;

    /// Instantly reset character position to new value.
    void WarpKinematic(const Vector3& position);

    void HandlePhysicsPreUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData);
    void HandlePhysicsPostUpdate(StringHash eventType, VariantMap& eventData);

    void SendCollisionEvent(StringHash physicsEvent, StringHash nodeEvent, RigidBody* otherBody);
    void ActivateTriggers();

protected:
    unsigned colLayer_{ 1 };
    unsigned colMask_{ 0xffff };

    float height_{ 1.8f };
    float diameter_{ 0.7f };
    float stepHeight_{ 0.4f };
    float maxJumpHeight_{ 2.0f };
    float jumpSpeed_{ 9.0f };
    float fallSpeed_{ 55.0f };
    float maxSlope_{ 45.0f };
    float linearDamping_{ 0.2f };
    float angularDamping_{ 0.2f };
    bool activateTriggers_ {true};
    Vector3 gravity_{ Vector3(0.0f, -14.0f, 0.0f) };

    WeakPtr<PhysicsWorld> physicsWorld_;
    /// Bullet collision shape.
    ea::unique_ptr<btCapsuleShape> shape_;
    ea::unique_ptr<btPairCachingGhostObject> pairCachingGhostObject_;
    ea::unique_ptr<btKinematicCharacterController> kinematicController_;
    ea::unordered_map<SharedPtr<RigidBody>, bool> activeTriggerContacts_;
    bool activeTriggerFlag_{};

    Vector3 colShapeOffset_{ 0.0f, 0.9f, 0.0f };
    bool readdToWorld_{ false };

    /// Offset used for smooth Node position adjustment.
    Vector3 positionOffset_;
    /// Constant used to fade out position offset.
    float smoothingConstant_{};
    /// Position used for interpolation and the final result.
    /// @{
    Vector3 previousPosition_;
    Vector3 nextPosition_;
    Vector3 latestPosition_;
    /// @}

    /// Cached event data for physics collision.
    VariantMap physicsCollisionData_;
    /// Cached event data for node collision.
    VariantMap nodeCollisionData_;
};

}
