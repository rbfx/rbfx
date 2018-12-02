#pragma once
#include "Constraint.h"


namespace Urho3D {
    class Context;
    class URHO3D_API FixedDistanceConstraint : public Constraint
    {
        URHO3D_OBJECT(FixedDistanceConstraint, Constraint);

    public:

        FixedDistanceConstraint(Context* context);
        ~FixedDistanceConstraint();

        static void RegisterObject(Context* context);


    protected:

        virtual void buildConstraint() override;
    };


}
