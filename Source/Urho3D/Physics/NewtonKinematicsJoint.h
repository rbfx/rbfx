#pragma once
#include "Constraint.h"




namespace Urho3D {
    class Context;

    ///Contraint for moving rigid bodies to a target position and orientation.
    class URHO3D_API KinematicsControllerConstraint : public Constraint
    {
        URHO3D_OBJECT(KinematicsControllerConstraint, Constraint);

    public:

        KinematicsControllerConstraint(Context* context);
        ~KinematicsControllerConstraint();

        static void RegisterObject(Context* context);

        /// Visualize the component as debug geometry.
        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

        ///Set Max Linear Friction - The higher this is the more powerful the joint will be but may exert too much force on other bodies.
        void SetLinearFrictionalAcceleration(float friction);
        ///Set Max Angular Friction - The higher this is the more powerful the joint will be but may exert too much force on other bodies.
        void SetAngularFrictionalAcceleration(float friction);

        ///Enforce rotational target. if disable only position will be constrained and the body will be free to rotate.
        void SetConstrainRotation(bool enable);

        /// Limit the rotation velocity to minimize instability. default true.
        void SetLimitRotationalVelocity(bool enable);


        virtual void SetOtherPosition(const Vector3& position) override;


        virtual void SetOtherRotation(const Quaternion& rotation) override;


    protected:

        virtual void buildConstraint() override;

        void updateTarget();

        void updateFrictions();


        ///If enabled the constraint will force orientation to the current target orientation.
        bool constrainRotation_ = true;

        ///If enabled the constraint will limit the rotational velocity. if false the joint may become unstable.
        bool limitRotationalVelocity_ = true;

        float linearFrictionalAcceleration = 1000.0f;
        float angularFrictionalAcceleration = 1000.0f;
    };
}
