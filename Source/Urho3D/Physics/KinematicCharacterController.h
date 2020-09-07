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

#include "../Scene/Component.h"

namespace Urho3D
{
class PhysicsWorld;
class DebugRenderer;
}

using namespace Urho3D;

class btPairCachingGhostObject;
class btKinematicCharacterController;

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

    const Vector3& GetPosition();
    const Quaternion& GetRotation();
    void SetTransform(const Vector3& position, const Quaternion& rotation);
    void GetTransform(Vector3& position, Quaternion& rotation);

    void SetCollisionLayer(int layer);
    void SetCollisionMask(int mask);
    void SetCollisionLayerAndMask(int layer, int mask);

    void SetGravity(const Vector3 &gravity);
    const Vector3& GetGravity() const { return gravity_; }
    void SetLinearDamping(float linearDamping);
    float GetLinearDamping() const { return linearDamping_; }
    void SetAngularDamping(float angularDamping);
    float GetAngularDamping() const { return angularDamping_; }

    void SetStepHeight(float stepHeight);
    float GetStepHeight() const { return stepHeight_; }
    void SetMaxJumpHeight(float maxJumpHeight);
    float GetMaxJumpHeight() const { return maxJumpHeight_; }
    void SetFallSpeed(float fallSpeed);
    float GetFallSpeed() const { return fallSpeed_; }
    void SetJumpSpeed(float jumpSpeed);
    float GetJumpSpeed() const { return jumpSpeed_; }
    void SetMaxSlope(float maxSlope);
    float GetMaxSlope() const { return maxSlope_; }

    void SetWalkDirection(const Vector3& walkDir);
    bool OnGround() const;
    void Jump(const Vector3 &jump = Vector3::ZERO);
    /// ApplyImpulse is same as Jump
    void ApplyImpulse(const Vector3 &impulse);
    bool CanJump() const;

    void SetAngularVelocity(const Vector3 &velocity);
    const Vector3 GetAngularVelocity() const;
    void SetLinearVelocity(const Vector3 &velocity);
    const Vector3 GetLinearVelocity() const;
    void Warp(const Vector3 &position);
    virtual void DrawDebugGeometry();

protected:
    void ReleaseKinematic();
    void ApplySettings(bool reapply=false);
    void OnNodeSet(Node* node) override;
    void OnSceneSet(Scene* scene) override;
    void AddKinematicToWorld();
    void RemoveKinematicFromWorld();

protected:
    int colLayer_{ 1 };
    int colMask_{ 0xffff };

    float stepHeight_{ 0.4f };
    float maxJumpHeight_{ 2.0f };
    float jumpSpeed_{ 9.0f };
    float fallSpeed_{ 55.0f };
    float maxSlope_{ 45.0f };
    float linearDamping_{ 0.2f };
    float angularDamping_{ 0.2f };
    Vector3 gravity_{ Vector3(0.0f, -14.0f, 0.0f) };

    WeakPtr<PhysicsWorld> physicsWorld_;
    ea::unique_ptr<btPairCachingGhostObject> pairCachingGhostObject_;
    ea::unique_ptr<btKinematicCharacterController> kinematicController_;

    Vector3 position_;
    Quaternion rotation_;
    Vector3 colShapeOffset_{ Vector3::ZERO };
    bool reapplyAttributes_{ false };

};
