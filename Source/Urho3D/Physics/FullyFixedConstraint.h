#pragma once



#include "Constraint.h"


namespace Urho3D {
    class Context;
    class URHO3D_API FullyFixedConstraint : public Constraint
    {
        URHO3D_OBJECT(FullyFixedConstraint, Constraint);

    public:

        FullyFixedConstraint(Context* context);
        ~FullyFixedConstraint();

        static void RegisterObject(Context* context);


    protected:

        virtual void buildConstraint() override;
    };


}
