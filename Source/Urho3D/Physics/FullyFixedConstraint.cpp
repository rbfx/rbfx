#include "FullyFixedConstraint.h"
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


    FullyFixedConstraint::FullyFixedConstraint(Context* context) : Constraint(context)
    {

    }

    FullyFixedConstraint::~FullyFixedConstraint()
    {

    }

    void FullyFixedConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<FullyFixedConstraint>(DEF_PHYSICS_CATEGORY.CString());

        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);
    }

    void FullyFixedConstraint::buildConstraint()
    {

        Matrix3x4 ownFrame = GetOwnNewtonWorldFrame();
        Matrix3x4 otherFrame = GetOtherNewtonWorldFrame();

        newtonJoint_ = new dCustom6dof(UrhoToNewton(ownFrame), UrhoToNewton(otherFrame), GetOwnNewtonBody(), GetOtherNewtonBody());

    }

}
