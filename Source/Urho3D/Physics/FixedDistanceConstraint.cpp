#include "FixedDistanceConstraint.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "Newton.h"
#include "dMatrix.h"
#include "../Physics/RigidBody.h"
#include "UrhoNewtonConversions.h"
#include "dCustomFixDistance.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"

namespace Urho3D {




    FixedDistanceConstraint::FixedDistanceConstraint(Context* context) : Constraint(context)
    {
    }

    FixedDistanceConstraint::~FixedDistanceConstraint()
    {
    }

    void FixedDistanceConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<FixedDistanceConstraint>(DEF_PHYSICS_CATEGORY.CString());


        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);



    }


    void FixedDistanceConstraint::buildConstraint()
{
        //get own body transform.
        dVector pivot0 = UrhoToNewton(GetOwnNewtonBuildWorldFrame().Translation());
        dVector pivot1 = UrhoToNewton(GetOtherNewtonBuildWorldFrame().Translation());
        




        newtonJoint_ = new dCustomFixDistance(pivot0, pivot1, GetOwnNewtonBody(), GetOtherNewtonBody());

    }

}
