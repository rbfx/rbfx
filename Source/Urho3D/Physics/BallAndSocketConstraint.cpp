#include "BallAndSocketConstraint.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "Newton.h"
#include "dMatrix.h"
#include "../Physics/RigidBody.h"
#include "UrhoNewtonConversions.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"
#include "dCustomBallAndSocket.h"
#include "dCustomJoint.h"
#include "dCustom6dof.h"
#include "NewtonDebugDrawing.h"
#include "Graphics/DebugRenderer.h"

namespace Urho3D {

    BallAndSocketConstraint::BallAndSocketConstraint(Context* context) : Constraint(context)
    {

    }

    void BallAndSocketConstraint::SetConeAngle(float angle)
    {
        if (coneAngle_ != angle) {
            coneAngle_ = angle;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetConeLimits(coneAngle_ * dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }

    float BallAndSocketConstraint::GetConeAngle() const
    {
        return coneAngle_;
    }

    void BallAndSocketConstraint::SetTwistLimits(float minAngle, float maxAngle)
    {
        if (twistMaxAngle_ != maxAngle || twistMinAngle_ != minAngle) {
            twistMinAngle_ = minAngle;
            twistMaxAngle_ = maxAngle;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetTwistLimits(twistMinAngle_* dDegreeToRad, twistMaxAngle_ * dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }

    void BallAndSocketConstraint::SetTwistLimitMin(float minAngle)
    {
        if (twistMinAngle_ != minAngle) {
            twistMinAngle_ = minAngle;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetTwistLimits(twistMinAngle_* dDegreeToRad, twistMaxAngle_ * dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }

    void BallAndSocketConstraint::SetTwistLimitMax(float maxAngle)
    {
        if (twistMaxAngle_ != maxAngle) {
            twistMaxAngle_ = maxAngle;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetTwistLimits(twistMinAngle_* dDegreeToRad, twistMaxAngle_ * dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }

    float BallAndSocketConstraint::GetTwistLimitMin() const
    {
        return twistMinAngle_;
    }

    float BallAndSocketConstraint::GetTwistLimitMax() const
    {
        return twistMaxAngle_;
    }

    Urho3D::Vector2 BallAndSocketConstraint::GetTwistLimits() const
    {
        return Vector2(twistMinAngle_, twistMaxAngle_);
    }

    void BallAndSocketConstraint::SetConeEnabled(bool enabled /*= true*/)
    {
        if (coneEnabled_ != enabled) {
            coneEnabled_ = enabled;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->EnableCone(coneEnabled_);
            }
            else
                MarkDirty();
        }
    }

    bool BallAndSocketConstraint::GetConeEnabled() const
    {
        return coneEnabled_;
    }

    void BallAndSocketConstraint::SetTwistLimitsEnabled(bool enabled /*= false*/)
    {
        if (twistLimitsEnabled_ != enabled) {
            twistLimitsEnabled_ = enabled;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->EnableTwist(twistLimitsEnabled_);
            }
            else
                MarkDirty();
        }
    }

    bool BallAndSocketConstraint::GetTwistLimitsEnabled() const
    {
        return twistLimitsEnabled_;
    }

    void BallAndSocketConstraint::SetConeFriction(float frictionTorque)
    {
        if (frictionTorque != coneFriction_) {
            coneFriction_ = frictionTorque;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetConeFriction(physicsWorld_->SceneToPhysics_Domain(coneFriction_));
            }
            else
                MarkDirty();
        }
    }

    float BallAndSocketConstraint::GetConeFriction() const
    {
        return coneFriction_;
    }

    void BallAndSocketConstraint::SetTwistFriction(float frictionTorque)
    {
        if (twistFriction_ != frictionTorque) {
            twistFriction_ = frictionTorque;
            if (newtonJoint_)
            {
                static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetTwistFriction(physicsWorld_->SceneToPhysics_Domain(twistFriction_));
            }
            else
                MarkDirty();
        }
    }

    float BallAndSocketConstraint::GetTwistFriction() const
    {
        return twistFriction_;
    }

    void BallAndSocketConstraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Constraint::DrawDebugGeometry(debug, depthTest);
    }



    Urho3D::BallAndSocketConstraint::~BallAndSocketConstraint()
    {

    }

    void Urho3D::BallAndSocketConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<BallAndSocketConstraint>(DEF_PHYSICS_CATEGORY.CString());


        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);

        URHO3D_ACCESSOR_ATTRIBUTE("Cone Enabled", GetConeEnabled, SetConeEnabled, bool, true, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Cone Angle", GetConeAngle, SetConeAngle, float, 20.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Cone Friction", GetConeFriction, SetConeFriction, float, 0.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Limits Enabled", GetTwistLimitsEnabled, SetTwistLimitsEnabled, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Angle Min", GetTwistLimitMin, SetTwistLimitMin, float, -45.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Angle Max", GetTwistLimitMax, SetTwistLimitMax, float, 45.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Friction", GetTwistFriction, SetTwistFriction, float, 0.0f, AM_DEFAULT);

    }

    void Urho3D::BallAndSocketConstraint::buildConstraint()
    {
        // Create a dCustomBallAndSocket
        newtonJoint_ = new dCustomBallAndSocket(UrhoToNewton(GetOwnNewtonBuildWorldFrame()), UrhoToNewton(GetOtherNewtonBuildWorldFrame()), GetOwnNewtonBody(), GetOtherNewtonBody());

        
    }
    bool BallAndSocketConstraint::applyAllJointParams()
    {
        if (!Constraint::applyAllJointParams())
            return false;

        static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetConeLimits(coneAngle_ * dDegreeToRad);
        static_cast<dCustomBallAndSocket*>(newtonJoint_)->EnableCone(coneEnabled_);
        static_cast<dCustomBallAndSocket*>(newtonJoint_)->EnableTwist(twistLimitsEnabled_);
        static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetTwistLimits(twistMinAngle_* dDegreeToRad, twistMaxAngle_ * dDegreeToRad);
        static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetConeFriction(physicsWorld_->SceneToPhysics_Domain(coneFriction_));
        static_cast<dCustomBallAndSocket*>(newtonJoint_)->SetTwistFriction(physicsWorld_->SceneToPhysics_Domain(twistFriction_));


        return true;
    }

}
