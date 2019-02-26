#include "6DOFConstraint.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "dCustom6dof.h"
#include "UrhoNewtonConversions.h"

namespace Urho3D {


    SixDofConstraint::SixDofConstraint(Context* context) : Constraint(context)
    {

    }

    SixDofConstraint::~SixDofConstraint()
    {

    }

    void SixDofConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<SixDofConstraint>(DEF_PHYSICS_CATEGORY.CString());


        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);
    }

    void SixDofConstraint::SetPitchLimits(float minLimit, float maxLimit)
    {
        if (pitchLimits_ != Vector2(minLimit, maxLimit)) {
            pitchLimits_.x_ = minLimit;
            pitchLimits_.y_ = maxLimit;
            if (newtonJoint_)
            {
                static_cast<dCustom6dof*>(newtonJoint_)->SetPitchLimits(pitchLimits_.x_, pitchLimits_.y_);
            }
            else
                MarkDirty();
        }
    }

    void SixDofConstraint::SetPitchLimits(const Vector3& limits)
    {
        SetPitchLimits(limits.x_, limits.y_);
    }

    void SixDofConstraint::SetYawLimits(float minLimit, float maxLimit)
    {
        if (yawLimits_ != Vector2(minLimit, maxLimit)) {
            yawLimits_.x_ = minLimit;
            yawLimits_.y_ = maxLimit;
            if (newtonJoint_)
            {
                static_cast<dCustom6dof*>(newtonJoint_)->SetYawLimits(yawLimits_.x_, yawLimits_.y_);
            }
            else
                MarkDirty();
        }
    }

    void SixDofConstraint::SetYawLimits(const Vector3& limits)
    {
        SetYawLimits(limits.x_, limits.y_);
    }

    void SixDofConstraint::SetRollLimits(float minLimit, float maxLimit)
    {
        if (rollLimits_ != Vector2(minLimit, maxLimit)) {
            rollLimits_.x_ = minLimit;
            rollLimits_.y_ = maxLimit;
            if (newtonJoint_)
            {
                static_cast<dCustom6dof*>(newtonJoint_)->SetRollLimits(rollLimits_.x_, rollLimits_.y_);
            }
            else
                MarkDirty();
        }
    }

    void SixDofConstraint::SetRollLimits(const Vector3& limits)
    {
        SetRollLimits(limits.x_, limits.y_);
    }

    void SixDofConstraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Constraint::DrawDebugGeometry(debug, depthTest);
    }

    void SixDofConstraint::buildConstraint()
    {
        newtonJoint_ = new dCustom6dof(UrhoToNewton(GetOwnNewtonBuildWorldFrame()), UrhoToNewton(GetOtherNewtonBuildWorldFrame()), GetOwnNewtonBody(), GetOtherNewtonBody());




    }

    bool SixDofConstraint::applyAllJointParams()
    {
        if (!Constraint::applyAllJointParams())
            return false;

        static_cast<dCustom6dof*>(newtonJoint_)->SetLinearLimits(dVector(M_LARGE_VALUE, M_LARGE_VALUE, M_LARGE_VALUE)*-1.0f, dVector(M_LARGE_VALUE, M_LARGE_VALUE, M_LARGE_VALUE));
        static_cast<dCustom6dof*>(newtonJoint_)->SetPitchLimits(pitchLimits_.x_ * dDegreeToRad, pitchLimits_.y_* dDegreeToRad);
        static_cast<dCustom6dof*>(newtonJoint_)->SetYawLimits(yawLimits_.x_* dDegreeToRad, yawLimits_.y_* dDegreeToRad);
        static_cast<dCustom6dof*>(newtonJoint_)->SetRollLimits(rollLimits_.x_* dDegreeToRad, rollLimits_.y_* dDegreeToRad);

        return true;
    }

}

