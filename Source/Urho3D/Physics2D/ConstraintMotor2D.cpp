// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Physics2D/ConstraintMotor2D.h"
#include "Urho3D/Physics2D/PhysicsUtils2D.h"
#include "Urho3D/Physics2D/RigidBody2D.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

ConstraintMotor2D::ConstraintMotor2D(Context* context) :
    Constraint2D(context),
    linearOffset_(Vector2::ZERO)
{

}

ConstraintMotor2D::~ConstraintMotor2D() = default;

void ConstraintMotor2D::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ConstraintMotor2D>(Category_Physics2D);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Linear Offset", GetLinearOffset, SetLinearOffset, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Angular Offset", GetAngularOffset, SetAngularOffset, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Force", GetMaxForce, SetMaxForce, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Torque", GetMaxTorque, SetMaxTorque, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Correction Factor", GetCorrectionFactor, SetCorrectionFactor, float, 0.3f, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintMotor2D::SetLinearOffset(const Vector2& linearOffset)
{
    if (linearOffset == linearOffset_)
        return;

    linearOffset_ = linearOffset;

    if (joint_)
        static_cast<b2MotorJoint*>(joint_)->SetLinearOffset(ToB2Vec2(linearOffset));
    else
        RecreateJoint();
}

void ConstraintMotor2D::SetAngularOffset(float angularOffset)
{
    if (angularOffset == jointDef_.angularOffset)
        return;

    jointDef_.angularOffset = angularOffset;

    if (joint_)
        static_cast<b2MotorJoint*>(joint_)->SetAngularOffset(angularOffset);
    else
        RecreateJoint();
}

void ConstraintMotor2D::SetMaxForce(float maxForce)
{
    if (maxForce == jointDef_.maxForce)
        return;

    jointDef_.maxForce = maxForce;

    if (joint_)
        static_cast<b2MotorJoint*>(joint_)->SetMaxForce(maxForce);
    else
        RecreateJoint();
}

void ConstraintMotor2D::SetMaxTorque(float maxTorque)
{
    if (maxTorque == jointDef_.maxTorque)
        return;

    jointDef_.maxTorque = maxTorque;

    if (joint_)
        static_cast<b2MotorJoint*>(joint_)->SetMaxTorque(maxTorque);
    else
        RecreateJoint();
}

void ConstraintMotor2D::SetCorrectionFactor(float correctionFactor)
{
    if (correctionFactor == jointDef_.correctionFactor)
        return;

    jointDef_.correctionFactor = correctionFactor;

    if (joint_)
        static_cast<b2MotorJoint*>(joint_)->SetCorrectionFactor(correctionFactor);
    else
        RecreateJoint();
}

b2JointDef* ConstraintMotor2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.Initialize(bodyA, bodyB);
    jointDef_.linearOffset = ToB2Vec2(linearOffset_);

    return &jointDef_;
}

}
