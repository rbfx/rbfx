#include "HingeConstraint.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "Newton.h"
#include "dMatrix.h"
#include "../Physics/RigidBody.h"
#include "UrhoNewtonConversions.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"
#include "dCustomJoint.h"
#include "NewtonDebugDrawing.h"
#include "Graphics/DebugRenderer.h"
#include "dCustomHinge.h"
#include "dCustomHingeActuator.h"
#include "dCustomMotor.h"
#include "IO/Log.h"

namespace Urho3D {

    const char* hingePoweredModeNames[] =
    {
        "NO_POWER",
        "MOTOR",
        "ACTUATOR",
        nullptr
    };


    HingeConstraint::HingeConstraint(Context* context) : Constraint(context)
    {

    }

    HingeConstraint::~HingeConstraint()
    {

    }

    void HingeConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<HingeConstraint>(DEF_PHYSICS_CATEGORY.CString());


        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);

        URHO3D_ACCESSOR_ATTRIBUTE("Enable Limits", GetLimitsEnabled, SetEnableLimits, bool, true, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Angle Min", GetMinAngle, SetMinAngle, float, -45.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Angle Max", GetMaxAngle, SetMaxAngle, float, 45.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Friction", GetFriction, SetFriction, float, 0.0f, AM_DEFAULT);
        URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Power Mode", GetPowerMode, SetPowerMode, PoweredMode, hingePoweredModeNames, PoweredMode::NO_POWER, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Max Torque", GetMaxTorque, SetMaxTorque, float, 10000.0f, AM_DEFAULT);

        URHO3D_ACCESSOR_ATTRIBUTE("Actuator Max Angular Rate", GetActuatorMaxAngularRate, SetActuatorMaxAngularRate, float, 1.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Actuator Target Angle", GetActuatorTargetAngle, SetActuatorTargetAngle, float, 0.0f, AM_DEFAULT);

        URHO3D_ACCESSOR_ATTRIBUTE("Spring Damper Enable", GetNoPowerSpringDamper, SetNoPowerSpringDamper, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Spring Coefficient", GetNoPowerSpringCoefficient, SetNoPowerSpringCoefficient, float, HINGE_CONSTRAINT_DEF_SPRING_COEF, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Spring Damper Coefficient", GetNoPowerSpringDamper, SetNoPowerSpringDamper, float, HINGE_CONSTRAINT_DEF_DAMPER_COEF, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Spring Damper Relaxation", GetNoPowerSpringDamperRelaxation, SetNoPowerSpringDamperRelaxation, float, HINGE_CONSTRAINT_DEF_RELAX, AM_DEFAULT);


    }

    void HingeConstraint::SetMinAngle(float minAngle)
    {
        if (minAngle_ != minAngle) {
            minAngle_ = minAngle;

            WakeBodies();

            if (newtonJoint_)
            {
                if (powerMode_ == NO_POWER)
                    static_cast<dCustomHinge*>(newtonJoint_)->SetLimits(minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad);
                if (powerMode_ == ACTUATOR)
                    static_cast<dCustomHingeActuator*>(newtonJoint_)->SetLimits(minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetMaxAngle(float maxAngle)
    {
        if (maxAngle_ != maxAngle) {
            maxAngle_ = maxAngle;
            WakeBodies();
            if (newtonJoint_) {
                if (powerMode_ == NO_POWER)
                    static_cast<dCustomHinge*>(newtonJoint_)->SetLimits(minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad);
                if (powerMode_ == ACTUATOR)
                    static_cast<dCustomHingeActuator*>(newtonJoint_)->SetLimits(minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetEnableLimits(bool enable)
    {
        if (enableLimits_ != enable) {
            enableLimits_ = enable;
            WakeBodies();
            if (newtonJoint_) {
                if (powerMode_ == NO_POWER)
                    static_cast<dCustomHinge*>(newtonJoint_)->EnableLimits(enableLimits_);
                if (powerMode_ == ACTUATOR)
                    static_cast<dCustomHingeActuator*>(newtonJoint_)->EnableLimits(enableLimits_);
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetFriction(float friction)
    {

        if (frictionTorque_ != friction) {
            frictionTorque_ = friction;
            WakeBodies();
            if (newtonJoint_) {
                if (powerMode_ == NO_POWER)
                    dynamic_cast<dCustomHinge*>(newtonJoint_)->SetFriction(physicsWorld_->SceneToPhysics_Domain_Torque(frictionTorque_));
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetMaxTorque(float torque)
    {
        if (maxTorque_ != torque)
        {
            maxTorque_ = torque;
            WakeBodies();
            if (newtonJoint_)
            {
                if (powerMode_ == ACTUATOR)
                    static_cast<dCustomHingeActuator*>(newtonJoint_)->SetMaxTorque(physicsWorld_->SceneToPhysics_Domain_Torque(maxTorque_));
                else if (powerMode_ == MOTOR)
                    static_cast<dCustomHinge*>(newtonJoint_)->SetFriction(physicsWorld_->SceneToPhysics_Domain_Torque(maxTorque_));
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetPowerMode(PoweredMode mode)
    {
        if (powerMode_ != mode) {
            powerMode_ = mode;
            MarkDirty();
        }
        else
            MarkDirty();
    }



    void HingeConstraint::SetActuatorMaxAngularRate(float rate)
    {
        if (maxAngularRate_ != rate)
        {
            maxAngularRate_ = rate;
            WakeBodies();
            if (newtonJoint_)
            {
                if (powerMode_ == ACTUATOR)
                    static_cast<dCustomHingeActuator*>(newtonJoint_)->SetAngularRate(maxAngularRate_);
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetActuatorTargetAngle(float angle)
    {
        if (targetAngle_ != angle)
        {
            targetAngle_ = angle;
            WakeBodies();
            if (newtonJoint_)
            {
                if (powerMode_ == ACTUATOR)
                    static_cast<dCustomHingeActuator*>(newtonJoint_)->SetTargetAngle(targetAngle_* dDegreeToRad);
            }
            else
                MarkDirty();
        }
    }


    void HingeConstraint::SetMotorTargetAngularRate(float rate)
    {
        if (maxAngularRate_ != rate)
        {
            maxAngularRate_ = rate;
            WakeBodies();
            if (newtonJoint_)
            {
                if (powerMode_ == MOTOR)
                    static_cast<dCustomHinge*>(newtonJoint_)->EnableMotor(true, maxAngularRate_);
            }
            else
                MarkDirty();
        }
    }

    void HingeConstraint::SetNoPowerSpringDamper(bool enable)
    {
        if (enableSpringDamper_ != enable)
        {
            enableSpringDamper_ = enable;

            if (newtonJoint_)
            {
                if (powerMode_ == NO_POWER)
                {
                    static_cast<dCustomHinge*>(newtonJoint_)->SetAsSpringDamper(enableSpringDamper_, springRelaxation_, springSpringCoef_, springDamperCoef_);
                }
            }
            else
                MarkDirty();

        }
    }



    void HingeConstraint::SetNoPowerSpringCoefficient(float springCoef)
    {
        if (springSpringCoef_ != springCoef)
        {
            springSpringCoef_ = springCoef;

            if (newtonJoint_)
            {
                if (powerMode_ == NO_POWER)
                {
                    static_cast<dCustomHinge*>(newtonJoint_)->SetAsSpringDamper(enableSpringDamper_, springRelaxation_, springSpringCoef_, springDamperCoef_);
                }
            }
            else
                MarkDirty();

        }
    }

    void HingeConstraint::SetNoPowerDamperCoefficient(float damperCoef)
    {
        if (springDamperCoef_ != damperCoef)
        {
            springDamperCoef_ = damperCoef;

            if (newtonJoint_)
            {
                if (powerMode_ == NO_POWER)
                {
                    static_cast<dCustomHinge*>(newtonJoint_)->SetAsSpringDamper(enableSpringDamper_, springRelaxation_, springSpringCoef_, springDamperCoef_);
                }
            }
            else
                MarkDirty();

        }
    }

    void HingeConstraint::SetNoPowerSpringDamperRelaxation(float relaxation)
    {
        if (springRelaxation_ != relaxation)
        {
            springRelaxation_ = relaxation;

            if (newtonJoint_)
            {
                if (powerMode_ == NO_POWER)
                {
                    static_cast<dCustomHinge*>(newtonJoint_)->SetAsSpringDamper(enableSpringDamper_, springRelaxation_, springSpringCoef_, springDamperCoef_);
                }
            }
            else
                MarkDirty();

        }
    }

    float HingeConstraint::GetCurrentAngularRate()
    {
        if (newtonJoint_)
        {
            return static_cast<dCustomHinge*>(newtonJoint_)->GetJointOmega();
        }
        return 0.0f;
    }

    float HingeConstraint::GetCurrentAngle()
    {
        if (newtonJoint_)
        {
            return static_cast<dCustomHinge*>(newtonJoint_)->GetJointAngle();
        }
        return 0.0f;
    }

    void HingeConstraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Constraint::DrawDebugGeometry(debug, depthTest);
    }

    void HingeConstraint::buildConstraint()
    {
        // Create a dCustomHinge

        if (powerMode_ == ACTUATOR)
        {
            newtonJoint_ = new dCustomHingeActuator(UrhoToNewton(GetOwnNewtonWorldFrame()), maxAngularRate_, minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad, GetOwnNewtonBody(), GetOtherNewtonBody());
        }
        else if (powerMode_ == MOTOR)
        {
            newtonJoint_ = new dCustomHinge(UrhoToNewton(GetOwnNewtonWorldFrame()), UrhoToNewton(GetOtherNewtonWorldFrame()), GetOwnNewtonBody(), GetOtherNewtonBody());
        }
        else
        {
            URHO3D_LOGINFO("Own World Frame:");
            PrintNewtonMatrix(UrhoToNewton(GetOwnNewtonWorldFrame()));
            URHO3D_LOGINFO("Body Matrix: ");
            PrintNewtonMatrix(UrhoToNewton(ownBody_->GetWorldTransform()));

            URHO3D_LOGINFO("Other World Frame:");
            PrintNewtonMatrix(UrhoToNewton(GetOtherNewtonWorldFrame()));
            URHO3D_LOGINFO("Own World Frame:");
            PrintNewtonMatrix(UrhoToNewton(otherBody_->GetWorldTransform()));

            newtonJoint_ = new dCustomHinge(UrhoToNewton(GetOwnNewtonWorldFrame()), UrhoToNewton(GetOtherNewtonWorldFrame()), GetOwnNewtonBody(), GetOtherNewtonBody());
        }


    }

    bool HingeConstraint::applyAllJointParams()
    {
        if (!Constraint::applyAllJointParams())
            return false;

        if (powerMode_ == ACTUATOR)
        {
            //static_cast<dCustomHingeActuator*>(newtonJoint_)->EnableLimits(enableLimits_); this breaks.
            static_cast<dCustomHingeActuator*>(newtonJoint_)->SetLimits(minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad);
            static_cast<dCustomHingeActuator*>(newtonJoint_)->SetTargetAngle(targetAngle_* dDegreeToRad);
            static_cast<dCustomHingeActuator*>(newtonJoint_)->SetMaxTorque(physicsWorld_->SceneToPhysics_Domain_Torque(maxTorque_));
            static_cast<dCustomHingeActuator*>(newtonJoint_)->SetAngularRate(maxAngularRate_);
        }
        else if (powerMode_ == MOTOR)
        {
            static_cast<dCustomHinge*>(newtonJoint_)->SetFriction(physicsWorld_->SceneToPhysics_Domain_Torque(maxTorque_));
            static_cast<dCustomHinge*>(newtonJoint_)->EnableMotor(true, maxAngularRate_);
        }
        else if(powerMode_ == NO_POWER)
        {
            static_cast<dCustomHinge*>(newtonJoint_)->EnableLimits(enableLimits_);
            static_cast<dCustomHinge*>(newtonJoint_)->SetLimits(minAngle_ * dDegreeToRad, maxAngle_ * dDegreeToRad);
            static_cast<dCustomHinge*>(newtonJoint_)->SetFriction(physicsWorld_->SceneToPhysics_Domain_Torque(frictionTorque_));
            static_cast<dCustomHinge*>(newtonJoint_)->SetAsSpringDamper(enableSpringDamper_, springRelaxation_, springSpringCoef_, springDamperCoef_);
        }


        return true;
    }



}
