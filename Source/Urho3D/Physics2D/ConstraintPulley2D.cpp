// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Physics2D/ConstraintPulley2D.h"
#include "Urho3D/Physics2D/PhysicsUtils2D.h"
#include "Urho3D/Physics2D/RigidBody2D.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

ConstraintPulley2D::ConstraintPulley2D(Context* context) :
    Constraint2D(context),
    ownerBodyGroundAnchor_(-1.0f, 1.0f),
    otherBodyGroundAnchor_(1.0f, 1.0f),
    ownerBodyAnchor_(-1.0f, 0.0f),
    otherBodyAnchor_(1.0f, 0.0f)
{

}

ConstraintPulley2D::~ConstraintPulley2D() = default;

void ConstraintPulley2D::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ConstraintPulley2D>(Category_Physics2D);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Owner Body Ground Anchor", GetOwnerBodyGroundAnchor, SetOwnerBodyGroundAnchor, Vector2, Vector2::ZERO,
        AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Other Body Ground Anchor", GetOtherBodyGroundAnchor, SetOtherBodyGroundAnchor, Vector2, Vector2::ZERO,
        AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Owner Body Anchor", GetOwnerBodyAnchor, SetOwnerBodyAnchor, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Other Body Anchor", GetOtherBodyAnchor, SetOtherBodyAnchor, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Ratio", GetRatio, SetRatio, float, 0.0f, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintPulley2D::SetOwnerBodyGroundAnchor(const Vector2& groundAnchor)
{
    if (groundAnchor == ownerBodyGroundAnchor_)
        return;

    ownerBodyGroundAnchor_ = groundAnchor;

    RecreateJoint();
}

void ConstraintPulley2D::SetOtherBodyGroundAnchor(const Vector2& groundAnchor)
{
    if (groundAnchor == otherBodyGroundAnchor_)
        return;

    otherBodyGroundAnchor_ = groundAnchor;

    RecreateJoint();
}

void ConstraintPulley2D::SetOwnerBodyAnchor(const Vector2& anchor)
{
    if (anchor == ownerBodyAnchor_)
        return;

    ownerBodyAnchor_ = anchor;

    RecreateJoint();
}

void ConstraintPulley2D::SetOtherBodyAnchor(const Vector2& anchor)
{
    if (anchor == otherBodyAnchor_)
        return;

    otherBodyAnchor_ = anchor;

    RecreateJoint();
}

void ConstraintPulley2D::SetRatio(float ratio)
{
    if (ratio == jointDef_.ratio)
        return;

    jointDef_.ratio = ratio;

    RecreateJoint();
}

void ConstraintPulley2D::OnSceneSet(Scene* previousScene, Scene* scene)
{
    Constraint2D::OnSceneSet(previousScene, scene);

    if (previousScene)
        UnsubscribeFromEvent(E_WORLDORIGINPOSTUPDATE);

    if (scene)
        SubscribeToEvent(scene, E_WORLDORIGINPOSTUPDATE, &ConstraintPulley2D::HandleWorldOriginPostUpdate);
}

void ConstraintPulley2D::HandleWorldOriginPostUpdate(VariantMap& eventData)
{
    using namespace WorldOriginPostUpdate;
    const Vector3 delta = eventData[P_DELTA].GetIntVector3().ToVector3();

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return;

    ownerBodyGroundAnchor_ -= delta.ToVector2();
    otherBodyGroundAnchor_ -= delta.ToVector2();
    ownerBodyAnchor_ = ToVector2(bodyA->GetWorldPoint(jointDef_.localAnchorA));
    otherBodyAnchor_ = ToVector2(bodyB->GetWorldPoint(jointDef_.localAnchorB));

    RecreateJoint();
}

b2JointDef* ConstraintPulley2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.Initialize(bodyA, bodyB, ToB2Vec2(ownerBodyGroundAnchor_), ToB2Vec2(otherBodyGroundAnchor_),
        ToB2Vec2(ownerBodyAnchor_), ToB2Vec2(otherBodyAnchor_), jointDef_.ratio);

    return &jointDef_;
}

}
