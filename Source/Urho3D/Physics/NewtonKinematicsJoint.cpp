#include "NewtonKinematicsJoint.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "Constraint.h"
#include "dCustomKinematicController.h"
#include "RigidBody.h"
#include "UrhoNewtonConversions.h"
#include "Graphics/DebugRenderer.h"
#include "IO/Log.h"



namespace Urho3D {




    KinematicsControllerConstraint::KinematicsControllerConstraint(Context* context) : Constraint(context)
    {
    }

    KinematicsControllerConstraint::~KinematicsControllerConstraint()
    {

    }

    void KinematicsControllerConstraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<KinematicsControllerConstraint>(DEF_PHYSICS_CATEGORY.CString());

        URHO3D_COPY_BASE_ATTRIBUTES(Constraint);
    }

    void KinematicsControllerConstraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Constraint::DrawDebugGeometry(debug, depthTest);
    }

    void KinematicsControllerConstraint::SetLinearFrictionalAcceleration(float friction)
    {
        if (linearFrictionalAcceleration != friction) {
            linearFrictionalAcceleration = friction;
            if(newtonJoint_)
                updateFrictions();
        }
    }

    void KinematicsControllerConstraint::SetAngularFrictionalAcceleration(float friction)
    {
        if (angularFrictionalAcceleration != friction) {
            angularFrictionalAcceleration = friction;
            if (newtonJoint_)
                updateFrictions();
        }
    }

    void KinematicsControllerConstraint::SetConstrainRotation(bool enable)
    {
        if (constrainRotation_ != enable)
        {
            constrainRotation_ = enable;
            if (newtonJoint_)
                static_cast<dCustomKinematicController*>(newtonJoint_)->SetPickMode(constrainRotation_);
        }
    }

    void KinematicsControllerConstraint::SetLimitRotationalVelocity(bool enable)
    {
        if (limitRotationalVelocity_ != enable)
        {
            limitRotationalVelocity_ = enable;
            if(newtonJoint_)
                static_cast<dCustomKinematicController*>(newtonJoint_)->SetLimitRotationVelocity(limitRotationalVelocity_);
        }
    }


    void KinematicsControllerConstraint::SetOtherPosition(const Vector3& position)
    {
        
        Constraint::SetOtherPosition(position);
        updateTarget();
        //dont dirty because otherPosition_ is used for target frame
    }

    void KinematicsControllerConstraint::SetOtherRotation(const Quaternion& rotation)
    {
        Constraint::SetOtherRotation(rotation);
        updateTarget();
        //dont dirty because otherRotation_ is used for target frame
    }

  

    void KinematicsControllerConstraint::buildConstraint()
    {
        newtonJoint_ = new dCustomKinematicController(GetOwnNewtonBody(), UrhoToNewton(GetOwnNewtonWorldFrame()));
        static_cast<dCustomKinematicController*>(newtonJoint_)->SetPickMode(constrainRotation_);
        updateFrictions();
        static_cast<dCustomKinematicController*>(newtonJoint_)->SetLimitRotationVelocity(limitRotationalVelocity_);


        updateTarget();
    }

    void KinematicsControllerConstraint::updateTarget()
    {
        if (newtonJoint_) {
            //GSS<VisualDebugger>()->AddCross(GetOtherWorldFrame().Translation(), 1.0f, Color::MAGENTA, false);
            //GSS<VisualDebugger>()->AddCross(GetOtherNewtonWorldFrame().Translation(), 2.0, Color::MAGENTA, false);
            static_cast<dCustomKinematicController*>(newtonJoint_)->SetTargetMatrix(UrhoToNewton(GetOtherNewtonWorldFrame()));
        }
    }

    void KinematicsControllerConstraint::updateFrictions()
    {

        dFloat Ixx;
        dFloat Iyy;
        dFloat Izz;
        dFloat mass;
        NewtonBodyGetMass(ownBody_->GetNewtonBody(), &mass, &Ixx, &Iyy, &Izz);


        const dFloat inertia = dMax(Izz, dMax(Ixx, Iyy));


        static_cast<dCustomKinematicController*>(newtonJoint_)->SetMaxLinearFriction(mass * linearFrictionalAcceleration);
        static_cast<dCustomKinematicController*>(newtonJoint_)->SetMaxAngularFriction(inertia * angularFrictionalAcceleration);
    }

}
