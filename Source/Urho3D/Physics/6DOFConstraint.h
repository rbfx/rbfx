#pragma once

#pragma once



#include "Constraint.h"


namespace Urho3D {
    class Context;




    class URHO3D_API SixDof_Constraint : public Constraint
    {
        URHO3D_OBJECT(SixDof_Constraint, Constraint);

    public:

        SixDof_Constraint(Context* context);
        ~SixDof_Constraint();



        static void RegisterObject(Context* context);


        ////Set the relative space the joint can occupy.v
        //void SetSpaceLimits();

        void SetPitchLimits(float minLimit, float maxLimit);
        
        void SetYawLimits(float minLimit, float maxLimit);

        void SetRollLimits(float minLimit, float maxLimit);





       
        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    protected:

        Vector2 pitchLimits_;
        Vector2 yawLimits_;
        Vector2 rollLimits_;



        virtual void buildConstraint() override;

        bool applyAllJointParams();
    };


}
