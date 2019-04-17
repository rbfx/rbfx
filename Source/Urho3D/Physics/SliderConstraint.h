#pragma once

#include "Constraint.h"

namespace Urho3D {



    class Context;

#define SLIDER_CONSTRAINT_DEF_SPRING_COEF 100.0f
#define SLIDER_CONSTRAINT_DEF_DAMPER_COEF 1.0f
#define SLIDER_CONSTRAINT_DEF_RELAX 0.9f

    class URHO3D_API SliderConstraint : public Constraint
    {
        URHO3D_OBJECT(SliderConstraint, Constraint);

    public:

        SliderConstraint(Context* context);
        ~SliderConstraint();


        static void RegisterObject(Context* context);


        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;




        void SetEnableSliderLimits(bool enableLowerLimit, bool enableUpperLimit);
        void SetSliderUpperLimitEnable(bool enable);
        bool GetSliderUpperLimitEnabled() const { return enableUpperSliderLimit_; }
        void SetSliderLowerLimitEnable(bool enable);
        bool GetSliderLowerLimitEnabled() const { return enableLowerSliderLimit_; }

        ///Set the distance limits the bodies with be able to slide. lower limit should be negative
        void SetSliderLimits(float lowerLimit, float upperLimit);
        void SetSliderUpperLimit(float upperLimit);
        float GetSliderUpperLimit() const { return sliderLimits_.y_; }
        void SetSliderLowerLimit(float lowerLimit);
        float GetSliderLowerLimit() const { return sliderLimits_.x_; }

        ///Set the friction for sliding.
        void SetSliderFriction(float friction);
        float GetSliderFriction() const { return sliderFriction_; }

        /// enable/disable spring damper on the linear (slide) motion of the constraint.
        void SetEnableSliderSpringDamper(bool enable);
        bool GetEnableSliderSpringDamper() const { return enableSliderSpringDamper_; }


        void SetSliderSpringCoefficient(float springCoef);
        float GetSliderSpringCoefficient() const { return sliderSpringCoef_; }

        void SetSliderDamperCoefficient(float damperCoef);
        float GetSliderDamperCoefficient() const { return sliderDamperCoef_; }

        void SetSliderSpringDamperRelaxation(float relaxation);
        float GetSliderSpringDamperRelaxation() const { return sliderRelaxation_; }






        ///Twist limits
        void SetEnableTwistLimits(bool enableUpperLimit, bool enableLowerLimit);
        void SetTwistUpperLimitEnable(bool enable);
        bool GetTwistUpperLimitEnabled() const { return enableUpperTwistLimit_; }
        void SetTwistLowerLimitEnable(bool enable);
        bool GetTwistLowerLimitEnabled() const { return enableLowerTwistLimit_; }



        void SetTwistLimits(float lowerLimit, float upperLimit);
        void SetTwistUpperLimit(float upperLimit);
        float GetTwistUpperLimit() const { return twistLimits_.y_; }
        void SetTwistLowerLimit(float lowerLimit);
        float GetTwistLowerLimit() const { return twistLimits_.x_; }


        void SetEnableTwistSpringDamper(bool enable);
        bool GetEnableTwistSpringDamper() const { return enableTwistSpringDamper_; }


        void SetTwistSpringCoefficient(float springCoef);
        float GetTwistSpringCoefficient() const { return twistSpringCoef_; }


        void SetTwistDamperCoefficient(float damperCoef);
        float GetTwistDamperCoefficient() const { return twistDamperCoef_; }


        void SetTwistSpringDamperRelaxation(float relaxation);
        float GetTwistSpringDamperRelaxation() const { return twistRelaxation_; }



    protected:

        bool enableLowerSliderLimit_ = false;
        bool enableUpperSliderLimit_ = false;
        Vector2 sliderLimits_;

        bool enableSliderSpringDamper_ = false;
        float sliderRelaxation_ = SLIDER_CONSTRAINT_DEF_RELAX;
        float sliderSpringCoef_ = SLIDER_CONSTRAINT_DEF_SPRING_COEF;
        float sliderDamperCoef_ = SLIDER_CONSTRAINT_DEF_DAMPER_COEF;

        float sliderFriction_ = 0.0f;



        bool enableLowerTwistLimit_ = false;
        bool enableUpperTwistLimit_ = false;
        Vector2 twistLimits_;

        bool enableTwistSpringDamper_ = false;
        float twistRelaxation_ = SLIDER_CONSTRAINT_DEF_RELAX;
        float twistSpringCoef_ = SLIDER_CONSTRAINT_DEF_SPRING_COEF;
        float twistDamperCoef_ = SLIDER_CONSTRAINT_DEF_DAMPER_COEF;


        void applySliderLimits();
        void applyTwistLimits();

        virtual void buildConstraint() override;

        bool applyAllJointParams();
    };



}
