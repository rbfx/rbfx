#include "6DOFConstraint.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "dCustom6dof.h"
#include "UrhoNewtonConversions.h"

namespace Urho3D {


    SixDof_Constraint::SixDof_Constraint(Context* context) : Constraint(context)
    {

    }

    SixDof_Constraint::~SixDof_Constraint()
    {

    }

    void SixDof_Constraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<SixDof_Constraint>(DEF_PHYSICS_CATEGORY.CString());


        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);
    }

    void SixDof_Constraint::SetPitchLimits(float minLimit, float maxLimit)
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

    void SixDof_Constraint::SetYawLimits(float minLimit, float maxLimit)
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

    void SixDof_Constraint::SetRollLimits(float minLimit, float maxLimit)
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

    void SixDof_Constraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Constraint::DrawDebugGeometry(debug, depthTest);
    }

    void SixDof_Constraint::buildConstraint()
    {
        newtonJoint_ = new dCustom6dof(UrhoToNewton(GetOwnNewtonWorldFrame()), UrhoToNewton(GetOtherNewtonWorldFrame()), GetOwnNewtonBody(), GetOtherNewtonBody());




    }

    bool SixDof_Constraint::applyAllJointParams()
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

