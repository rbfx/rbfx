#pragma once



#include "Constraint.h"


namespace Urho3D {
    class Context;


#define HINGE_CONSTRAINT_DEF_SPRING_COEF 100.0f
#define HINGE_CONSTRAINT_DEF_DAMPER_COEF 1.0f
#define HINGE_CONSTRAINT_DEF_RELAX 0.9f

    class URHO3D_API HingeConstraint : public Constraint
    {
        URHO3D_OBJECT(HingeConstraint, Constraint);

    public:

        HingeConstraint(Context* context);
        ~HingeConstraint();

        enum PoweredMode
        {
            NO_POWER = 0,
            MOTOR,
            ACTUATOR
        };


        static void RegisterObject(Context* context);

        void SetMinAngle(float minAngle);
        float GetMinAngle() const { return minAngle_; }

        void SetMaxAngle(float maxAngle);
        float GetMaxAngle() const { return maxAngle_; }

        void SetEnableLimits(bool enable);
        bool GetLimitsEnabled() const { return enableLimits_; }


        void SetFriction(float friction);
        float GetFriction() const { return frictionTorque_; }

        /// Set max torque for all powered modes.
        void SetMaxTorque(float torque);
        float GetMaxTorque() const { return maxTorque_; }

        /// enable/disable spring damper when the hinge is not powered.
        void SetNoPowerSpringDamper(bool enable);
        bool GetNoPowerSpringDamper() const { return enableSpringDamper_; }

        void SetNoPowerSpringCoefficient(float springCoef);
        float GetNoPowerSpringCoefficient() const { return springSpringCoef_; }

        void SetNoPowerDamperCoefficient(float damperCoef);
        float GetNoPowerDamperCoefficient() const { return springDamperCoef_; }

        void SetNoPowerSpringDamperRelaxation(float relaxation);
        float GetNoPowerSpringDamperRelaxation() const { return springRelaxation_; }



        float GetCurrentAngularRate();
        float GetCurrentAngle();


        ///set the hinge power mode
        void SetPowerMode(PoweredMode mode);
        PoweredMode GetPowerMode() const { return powerMode_; }




        ///actuator specific:
        void SetActuatorMaxAngularRate(float rate);
        float GetActuatorMaxAngularRate() const { return maxAngularRate_; }

        void SetActuatorTargetAngle(float angle);
        float GetActuatorTargetAngle() const { return targetAngle_; }


        ///motor specific:
        void SetMotorTargetAngularRate(float rate);




        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    protected:

        PoweredMode powerMode_ = NO_POWER;

        float frictionTorque_ = 0.0f;
        bool  enableLimits_ = true;
        float minAngle_ = -45.0f;
        float maxAngle_ = 45.0f;

        float enableSpringDamper_ = false;
        float springRelaxation_ = HINGE_CONSTRAINT_DEF_RELAX;
        float springSpringCoef_ = HINGE_CONSTRAINT_DEF_SPRING_COEF;
        float springDamperCoef_ = HINGE_CONSTRAINT_DEF_DAMPER_COEF;



        float maxTorque_ = 10000.0f;
        float maxAngularRate_ = 1.0f;
        float targetAngle_ = 0.0f;

        virtual void buildConstraint() override;

        bool applyAllJointParams();
    };


}
