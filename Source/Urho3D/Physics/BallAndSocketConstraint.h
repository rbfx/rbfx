#pragma once


#include "Constraint.h"


namespace Urho3D {
    class Context;

    

    class URHO3D_API BallAndSocketConstraint : public Constraint
    {
        URHO3D_OBJECT(BallAndSocketConstraint, Constraint);

    public:

        BallAndSocketConstraint(Context* context);
        ~BallAndSocketConstraint();

        static void RegisterObject(Context* context);


        ///Set Twist limits
        void SetTwistLimitsEnabled(bool enabled = false);
        bool GetTwistLimitsEnabled() const;
 
        void SetTwistLimits(float minAngle, float maxAngle);
        void SetTwistLimitMin(float minAngle);
        void SetTwistLimitMax(float maxAngle);

        float GetTwistLimitMin() const;
        float GetTwistLimitMax() const;
        Vector2 GetTwistLimits() const;

        void SetTwistFriction(float frictionTorque);
        float GetTwistFriction() const;



        ///enable or disable limiting cone
        void SetConeEnabled(bool enabled = true);
        bool GetConeEnabled() const;

        void SetConeAngle(float angle);
        float GetConeAngle() const;

        void SetConeFriction(float frictionTorque);
        float GetConeFriction() const;





        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    protected:
        bool coneEnabled_ = true;
        bool twistLimitsEnabled_ = false;
        float coneAngle_ = 20.0f;
        float twistMinAngle_ = -45.0f;
        float twistMaxAngle_ = 45.0f;
        float twistFriction_ = 0.0f;
        float coneFriction_ = 0.0f;

        virtual void buildConstraint() override;

        virtual bool applyAllJointParams() override;

    };


}
