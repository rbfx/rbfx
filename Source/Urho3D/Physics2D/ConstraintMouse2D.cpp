// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Physics2D/ConstraintMouse2D.h"
#include "../Physics2D/PhysicsUtils2D.h"
#include "../Physics2D/RigidBody2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

ConstraintMouse2D::ConstraintMouse2D(Context* context) :
    Constraint2D(context),
    target_(Vector2::ZERO)
{
}

ConstraintMouse2D::~ConstraintMouse2D() = default;

void ConstraintMouse2D::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ConstraintMouse2D>(Category_Physics2D);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Target", GetTarget, SetTarget, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Force", GetMaxForce, SetMaxForce, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Frequency Hz", GetFrequencyHz, SetFrequencyHz, float, 5.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Damping Ratio", GetDampingRatio, SetDampingRatio, float, 0.7f, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintMouse2D::SetTarget(const Vector2& target)
{
    if (target == target_)
        return;

    target_ = target;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetTarget(ToB2Vec2(target));
    else
        RecreateJoint();
}

void ConstraintMouse2D::SetMaxForce(float maxForce)
{
    if (maxForce == jointDef_.maxForce)
        return;

    jointDef_.maxForce = maxForce;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetMaxForce(maxForce);
    else
        RecreateJoint();
}

void ConstraintMouse2D::SetFrequencyHz(float frequencyHz)
{
    if (frequencyHz == jointDef_.frequencyHz)
        return;

    jointDef_.frequencyHz = frequencyHz;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetFrequency(frequencyHz);
    else
        RecreateJoint();
}

void ConstraintMouse2D::SetDampingRatio(float dampingRatio)
{
    if (dampingRatio == jointDef_.dampingRatio)
        return;

    jointDef_.dampingRatio = dampingRatio;

    if (joint_)
        static_cast<b2MouseJoint*>(joint_)->SetDampingRatio(dampingRatio);
    else
        RecreateJoint();
}

b2JointDef* ConstraintMouse2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = otherBody_->GetBody();
    b2Body* bodyB = ownerBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.bodyA = bodyA;
    jointDef_.bodyB = bodyB;
    jointDef_.collideConnected = collideConnected_;

    jointDef_.target = ToB2Vec2(target_);

    return &jointDef_;
}

}
