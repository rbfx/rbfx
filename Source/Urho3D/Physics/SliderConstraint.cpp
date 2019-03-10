#include "SliderConstraint.h"
#include "Core/Context.h"
#include "PhysicsWorld.h"
#include "dCustomSlider.h"
#include "UrhoNewtonConversions.h"
#include "dCustomCorkScrew.h"
#include "IO/Log.h"

namespace Urho3D
{
    SliderConstraint::SliderConstraint(Context* context) : Constraint(context)
    {

    }

    SliderConstraint::~SliderConstraint()
    {

    }

    void SliderConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<SliderConstraint>(DEF_PHYSICS_CATEGORY.CString());


        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Upper Limit Enable", GetSliderUpperLimitEnabled, SetSliderUpperLimitEnable, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Lower Limit Enable", GetSliderLowerLimitEnabled, SetSliderLowerLimitEnable, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Upper Limit", GetSliderUpperLimit, SetSliderUpperLimit, float, 0.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Lower Limit", GetSliderLowerLimit, SetSliderLowerLimit, float, 0.0f, AM_DEFAULT);

        URHO3D_ACCESSOR_ATTRIBUTE("Slider Spring Damper Enable", GetEnableSliderSpringDamper, SetEnableSliderSpringDamper, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Spring Coefficient", GetSliderSpringCoefficient, SetSliderSpringCoefficient, float, SLIDER_CONSTRAINT_DEF_SPRING_COEF, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Spring Damper Coefficient", GetSliderDamperCoefficient, SetSliderDamperCoefficient, float, SLIDER_CONSTRAINT_DEF_DAMPER_COEF, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Slider Spring Damper Relaxation", GetSliderSpringDamperRelaxation, SetSliderSpringDamperRelaxation, float, SLIDER_CONSTRAINT_DEF_RELAX, AM_DEFAULT);


        URHO3D_ACCESSOR_ATTRIBUTE("Twist Upper Limit Enable", GetTwistUpperLimitEnabled, SetTwistUpperLimitEnable, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Lower Limit Enable", GetTwistLowerLimitEnabled, SetTwistLowerLimitEnable, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Upper Limit", GetTwistUpperLimit, SetTwistUpperLimit, float, 0.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Lower Limit", GetTwistLowerLimit, SetTwistLowerLimit, float, 0.0f, AM_DEFAULT);

        URHO3D_ACCESSOR_ATTRIBUTE("Twist Spring Damper Enable", GetEnableTwistSpringDamper, SetEnableTwistSpringDamper, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Spring Coefficient", GetTwistSpringCoefficient, SetTwistSpringCoefficient, float, SLIDER_CONSTRAINT_DEF_SPRING_COEF, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Spring Damper Coefficient", GetTwistDamperCoefficient, SetTwistDamperCoefficient, float, SLIDER_CONSTRAINT_DEF_DAMPER_COEF, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Twist Spring Damper Relaxation", GetTwistSpringDamperRelaxation, SetTwistSpringDamperRelaxation, float, SLIDER_CONSTRAINT_DEF_RELAX, AM_DEFAULT);






    }

    void SliderConstraint::SetEnableSliderLimits(bool enableLowerLimit, bool enableUpperLimit)
    {
        SetSliderUpperLimitEnable(enableUpperLimit);
        SetSliderLowerLimitEnable(enableLowerLimit);

    }



    void SliderConstraint::SetSliderUpperLimitEnable(bool enable)
    {
        if (enableUpperSliderLimit_ != enable)
        {
            enableUpperSliderLimit_ = enable;
            if (newtonJoint_)
            {
                static_cast<dCustomSlider*>(newtonJoint_)->EnableLimits(enableLowerSliderLimit_ || enableUpperSliderLimit_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetSliderLowerLimitEnable(bool enable)
    {
        if (enableLowerSliderLimit_ != enable)
        {
            enableLowerSliderLimit_ = enable;
            if (newtonJoint_)
            {
                static_cast<dCustomSlider*>(newtonJoint_)->EnableLimits(enableLowerSliderLimit_ || enableUpperSliderLimit_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetSliderLimits(float lowerLimit, float upperLimit)
    {
        SetSliderLowerLimit(lowerLimit);
        SetSliderUpperLimit(upperLimit);
    }

    void SliderConstraint::SetSliderUpperLimit(float upperLimit)
    {
        if (sliderLimits_.y_ != upperLimit)
        {
            sliderLimits_.y_ = upperLimit;
            if (newtonJoint_)
            {
                applySliderLimits();
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetSliderLowerLimit(float lowerLimit)
    {
        if (sliderLimits_.x_ != lowerLimit)
        {
            sliderLimits_.x_ = lowerLimit;
            if (newtonJoint_)
            {
                applySliderLimits();
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetEnableSliderSpringDamper(bool enable)
    {
        if (enableSliderSpringDamper_ != enable)
        {
            enableSliderSpringDamper_ = enable;
            if (newtonJoint_)
            {
                static_cast<dCustomSlider*>(newtonJoint_)->SetAsSpringDamper(enableSliderSpringDamper_, sliderRelaxation_, sliderSpringCoef_, sliderDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetSliderSpringCoefficient(float springCoef)
    {
        if (sliderSpringCoef_ != springCoef)
        {
            sliderSpringCoef_ = springCoef;
            if (newtonJoint_)
            {
                static_cast<dCustomSlider*>(newtonJoint_)->SetAsSpringDamper(enableSliderSpringDamper_, sliderRelaxation_, sliderSpringCoef_, sliderDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetSliderDamperCoefficient(float damperCoef)
    {
        if (sliderDamperCoef_ != damperCoef)
        {
            sliderDamperCoef_ = damperCoef;
            if (newtonJoint_)
            {
                static_cast<dCustomSlider*>(newtonJoint_)->SetAsSpringDamper(enableSliderSpringDamper_, sliderRelaxation_, sliderSpringCoef_, sliderDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetSliderSpringDamperRelaxation(float relaxation)
    {
        if (sliderRelaxation_ != relaxation)
        {
            sliderRelaxation_ = relaxation;
            if (newtonJoint_)
            {
                static_cast<dCustomSlider*>(newtonJoint_)->SetAsSpringDamper(enableSliderSpringDamper_, sliderRelaxation_, sliderSpringCoef_, sliderDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetEnableTwistLimits(bool enableUpperLimit, bool enableLowerLimit)
    {
        SetTwistUpperLimitEnable(enableUpperLimit);
        SetTwistLowerLimitEnable(enableLowerLimit);

    }

    void SliderConstraint::SetTwistUpperLimitEnable(bool enable)
    {
        if (enableUpperTwistLimit_ != enable)
        {
            enableUpperTwistLimit_ = enable;
            if (newtonJoint_)
            {
                static_cast<dCustomCorkScrew*>(newtonJoint_)->EnableAngularLimits(enableLowerTwistLimit_ || enableUpperTwistLimit_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetTwistLowerLimitEnable(bool enable)
    {
        if (enableLowerTwistLimit_ != enable)
        {
            enableLowerTwistLimit_ = enable;
            if (newtonJoint_)
            {
                static_cast<dCustomCorkScrew*>(newtonJoint_)->EnableAngularLimits(enableLowerTwistLimit_ || enableUpperTwistLimit_);
            }
            else
                MarkDirty();
        }

    }

    void SliderConstraint::SetTwistLimits(float lowerLimit, float upperLimit)
    {
        SetTwistLowerLimit(lowerLimit);
        SetTwistUpperLimit(upperLimit);
    }

    void SliderConstraint::SetTwistUpperLimit(float upperLimit)
    {
        if (twistLimits_.y_ != upperLimit)
        {
            twistLimits_.y_ = upperLimit;
            if (newtonJoint_)
            {
                applyTwistLimits();
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetTwistLowerLimit(float lowerLimit)
    {
        if (twistLimits_.x_ != lowerLimit)
        {
            twistLimits_.x_ = lowerLimit;
            if (newtonJoint_)
            {
                applyTwistLimits();
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetEnableTwistSpringDamper(bool enable)
    {
        if (enableTwistSpringDamper_ != enable)
        {
            enableTwistSpringDamper_ = enable;
            if (newtonJoint_)
            {
                static_cast<dCustomCorkScrew*>(newtonJoint_)->SetAngularSpringDamper(enableTwistSpringDamper_, twistRelaxation_, twistSpringCoef_, twistDamperCoef_);

            }
            else
                MarkDirty();
        }
    }


    void SliderConstraint::SetTwistSpringCoefficient(float springCoef)
    {
        if (twistSpringCoef_ != springCoef)
        {
            twistSpringCoef_ = springCoef;
            if (newtonJoint_)
            {
                static_cast<dCustomCorkScrew*>(newtonJoint_)->SetAngularSpringDamper(enableTwistSpringDamper_, twistRelaxation_, twistSpringCoef_, twistDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetTwistDamperCoefficient(float damperCoef)
    {
        if (twistDamperCoef_ != damperCoef)
        {
            twistDamperCoef_ = damperCoef;
            if (newtonJoint_)
            {
                static_cast<dCustomCorkScrew*>(newtonJoint_)->SetAngularSpringDamper(enableTwistSpringDamper_, twistRelaxation_, twistSpringCoef_, twistDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void SliderConstraint::SetTwistSpringDamperRelaxation(float relaxation)
    {
        if (twistRelaxation_ != relaxation)
        {
            twistRelaxation_ = relaxation;
            if (newtonJoint_)
            {
                static_cast<dCustomCorkScrew*>(newtonJoint_)->SetAngularSpringDamper(enableTwistSpringDamper_, twistRelaxation_, twistSpringCoef_, twistDamperCoef_);
            }
            else
                MarkDirty();
        }
    }

    void Urho3D::SliderConstraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Constraint::DrawDebugGeometry(debug, depthTest);
    }

    void Urho3D::SliderConstraint::buildConstraint()
    {
        newtonJoint_ = new dCustomCorkScrew(UrhoToNewton(GetOwnNewtonBuildWorldFrame()), UrhoToNewton(GetOtherNewtonBuildWorldFrame()), GetOwnNewtonBody(), GetOtherNewtonBody());
    }

    bool Urho3D::SliderConstraint::applyAllJointParams()
    {
        if (!Constraint::applyAllJointParams())
            return false;






        
        static_cast<dCustomSlider*>(newtonJoint_)->SetAsSpringDamper(enableSliderSpringDamper_, sliderRelaxation_, sliderSpringCoef_, sliderDamperCoef_);




        static_cast<dCustomCorkScrew*>(newtonJoint_)->EnableAngularLimits(enableLowerTwistLimit_ || enableUpperTwistLimit_);
        applyTwistLimits();

        //#todo - this springdamper doesnt seem to work.
        static_cast<dCustomCorkScrew*>(newtonJoint_)->SetAngularSpringDamper(enableTwistSpringDamper_, twistRelaxation_, twistSpringCoef_, twistDamperCoef_);


        static_cast<dCustomSlider*>(newtonJoint_)->EnableLimits(enableLowerSliderLimit_ || enableUpperSliderLimit_);

        applySliderLimits();



        return true;
    }



    void SliderConstraint::applySliderLimits()
    {
        
        ///because we dont have control over upper vs lower limit on the dCustomSlider api right now - use +- infinity for no limit.
        float lowerSlideLimit = -FLT_MAX;
        float upperSlideLimit = FLT_MAX;
        if (enableLowerSliderLimit_)
            lowerSlideLimit = sliderLimits_.x_;
        if (enableUpperSliderLimit_)
            upperSlideLimit = sliderLimits_.y_;

        static_cast<dCustomSlider*>(newtonJoint_)->SetLimits(lowerSlideLimit, upperSlideLimit);
    }

    void SliderConstraint::applyTwistLimits()
    {
        ///because we dont have control over upper vs lower limit on the dCustomCorkScrew api right now - use +- infinity for no limit.
        float lowerTwistLimit = -FLT_MAX;
        float upperTwistLimit = FLT_MAX;
        if (enableLowerTwistLimit_)
            lowerTwistLimit = twistLimits_.x_;
        if (enableUpperTwistLimit_)
            upperTwistLimit = twistLimits_.y_;
        static_cast<dCustomCorkScrew*>(newtonJoint_)->SetAngularLimits(lowerTwistLimit * dDegreeToRad, upperTwistLimit * dDegreeToRad);
    }

}



