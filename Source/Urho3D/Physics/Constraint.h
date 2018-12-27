#pragma once
#include "../Scene/Component.h"


class dCustomJoint;
class NewtonBody;
namespace Urho3D {

    class Context;
    class RigidBody;
    class PhysicsWorld;

    enum CONSTRAINT_SOLVE_MODE {
        SOLVE_MODE_JOINT_DEFAULT = 0,     //Usually the best option - uses whatever solver mode newton has for the internal joint.
        SOLVE_MODE_EXACT = 1,             //Always use exact solving.
        SOLVE_MODE_ITERATIVE = 2,         //iterative solving use for a joint that forms a loop.
        SOLVE_MODE_KINEMATIC_LOOP = 3     //use this to specify a joint that is a connecting joint in a loop of joints. Only one joint should neeed to be in this solve mode.
    };
    ///Base class for newton constraints.
    class URHO3D_API Constraint : public Component
    {
        URHO3D_OBJECT(Constraint, Component);


    public:

        friend class PhysicsWorld;
        friend class RigidBody;


        /// Construct.
        Constraint(Context* context);
        /// Destruct. Free the rigid body and geometries.
        ~Constraint() override;

        static void RegisterObject(Context* context);

        /// Visualize the component as debug geometry.
        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

        void MarkDirty(bool dirty = true) { needsRebuilt_ = dirty; }

        /// Set whether to disable collisions between connected bodies.
        void SetDisableCollision(bool disable);

        /// Set other body to connect to. Set to null to connect to the static world.
        virtual void SetOtherBody(RigidBody* body);

        void SetOtherBodyById(unsigned bodyId);

        /// force wake the connected bodies
        void WakeBodies();

        ///set the world position of both frames on both bodies. make sure you set other body before calling this function
        void SetWorldPosition(const Vector3& position);
        ///set the world rotation of both frames on both bodies. make sure you set other body before calling this function
        void SetWorldRotation(const Quaternion& rotation);

        
        /// Set constraint position in local cordinates to rigidbody.
        void SetOwnPosition(const Vector3& position);
        /// set the rotational frame to use on own rigidbody 
        void SetOwnRotation(const Quaternion& rotation);

        void SetOwnWorldPosition(const Vector3& worldPosition);

        void SetOwnWorldRotation(const Quaternion& worldRotation);


        Vector3 GetOwnPosition() const { return position_; }

        Quaternion GetOwnRotation() const { return rotation_; }



        /// Set constraint position in local cordinates relative to the other body. If connected to the static world, is a world space position.
        virtual void SetOtherPosition(const Vector3& position);
        /// set the rotational frame to use on other body. If connected to the static world, is a world space position.
        virtual void SetOtherRotation(const Quaternion& rotation);

        /// Set constraint position in local cordinates relative to the other body. If connected to the static world, is a world space position.
        virtual void SetOtherWorldPosition(const Vector3& position);
        /// set the rotational frame to use on other body. If connected to the static world, is a world space position.
        virtual void SetOtherWorldRotation(const Quaternion& rotation);


        void SetSolveMode(CONSTRAINT_SOLVE_MODE mode) {
            if (solveMode_ != mode) {
                solveMode_ = mode;
                applyAllJointParams();
            }
        }
        void SetSolveMode(int mode) {
            SetSolveMode(CONSTRAINT_SOLVE_MODE(mode));
        }

        CONSTRAINT_SOLVE_MODE GetSolveMode() const { return solveMode_; }


        void SetStiffness(float stiffness) {
            if (stiffness_ != stiffness) {
                stiffness_ = stiffness;
                applyAllJointParams();
            }
        }
        float GetStiffness() const { return stiffness_; }


        /// Return physics world.
        PhysicsWorld* GetPhysicsWorld() const { return physicsWorld_; }

        /// Return rigid body in own scene node.
        RigidBody* GetOwnBody() const { return ownBody_; }

        NewtonBody* GetOwnNewtonBody() const;

        /// Return the other rigid body. May be null if connected to the static world.
        RigidBody* GetOtherBody() const { return otherBody_; }

        NewtonBody* GetOtherNewtonBody() const;

        unsigned GetOtherBodyId() const { return otherBodyId_; }


        Vector3 GetOtherPosition() const;

        Quaternion GetOtherRotation() const;


        Matrix3x4 GetOwnWorldFrame() const;


        Matrix3x4 GetOtherWorldFrame() const;

        dCustomJoint* GetNewtonJoint() const {
            return  newtonJoint_;
        }

        virtual void OnSetEnabled() override;

    protected:
        /// Physics world.
        WeakPtr<PhysicsWorld> physicsWorld_;
        /// Own rigid body.
        WeakPtr<RigidBody> ownBody_;
        unsigned ownBodyId_ = 0;
        /// Other rigid body.
        WeakPtr<RigidBody> otherBody_;
        unsigned otherBodyId_ = 0;
        /// Internal newtonJoint.
        dCustomJoint* newtonJoint_ = nullptr;
        /// Flag indicating the two bodies should collide with each other.
        bool enableBodyCollision_ = false;
        /// Constraint other body position.
        Vector3 otherPosition_;
        Quaternion otherRotation_;


        float stiffness_ = 0.7f;

        CONSTRAINT_SOLVE_MODE solveMode_ = SOLVE_MODE_JOINT_DEFAULT;

        /// Constraint position.
        Vector3 position_;
        Quaternion rotation_;

        ///dirty flag.
        bool needsRebuilt_ = true;

        bool otherFrameWorldExplicitlySet = false;

        bool reEvalOtherBodyFrame_ = false;
        Vector3 pendingOtherBodyFramePos_;




        /// Upper level re-evaulation.
        void reEvalConstraint();
        
        /// build the newton constraint.
        virtual void buildConstraint();

        /// update params on the already build constraint
        virtual bool applyAllJointParams();
        
        /// frees and deletes the internal joint.
        void freeInternal();

        void AddJointReferenceToBody(RigidBody* rigBody);
        void RemoveJointReferenceFromBody(RigidBody* rigBody);



        virtual void OnNodeSet(Node* node) override;
        virtual void OnNodeSetEnabled(Node* node) override;

        ///return the world frame on own body in newton world cordinates. (  use in buildConstraint method  )
        Matrix3x4 GetOwnNewtonWorldFrame();
        ///return the world frame on other body in newton world cordinates.(  use in buildConstraint method  )
        Matrix3x4 GetOtherNewtonWorldFrame();
        ///return the world pin direction for own body frame in newton world cordinates. (use in buildConstraint method)
        Vector3 GetOwnNewtonWorldPin();
        ///return the world pin direction for other body frame in newton world cordinates. (use in buildConstraint method)
        Vector3 GetOtherNewtonWorldPin();
    };
}

