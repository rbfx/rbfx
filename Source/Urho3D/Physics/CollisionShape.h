#pragma once

#include "../Scene/Component.h"

class NewtonCollision;
class NewtonMesh;
namespace Urho3D
{


    class NewtonNodePhysicsGlue;
    class PhysicsWorld;
    class RigidBody;
    class NewtonMeshObject;
    class Component;
    class Model;

#define COLLISION_SHAPE_DEF_STATIC_FRICTION 0.4f
#define COLLISION_SHAPE_DEF_KINETIC_FRICTION 0.5f
#define COLLISION_SHAPE_DEF_ELASTICITY 0.5f
#define COLLISION_SHAPE_DEF_SOFTNESS 0.5f

    /// base component for attaching collision shapes to nodes.
    class URHO3D_API CollisionShape : public Component
    {
        URHO3D_OBJECT(CollisionShape, Component);
    public:

        friend class PhysicsWorld;
        friend class RigidBody;

        CollisionShape(Context* context);

        virtual ~CollisionShape();

        static void RegisterObject(Context* context);

        
        /// Set the frictional value of the surface of the shape - automatically sets static and kinetic friction
        void SetFriction(float friction);

        /// Set the Static Frictional Coefficient of the shape
        void SetStaticFriction(float staticFriction);
        float GetStaticFriction() const;

        /// Set the Kinetic fricional coefficient of the shape
        void SetKineticFriction(float kineticFriction);
        float GetKineticFriction() const;


        /// Set the elasticity
        void SetElasticity(float elasticity);
        float GetElasticity() const;

        /// Set the softness
        void SetSoftness(float softness);
        float GetSoftness() const;

        /// Set the density scale.
        void SetDensity(float density);
        float GetDensity() const {
            return density_;
        }


        /// Set the positional offset of the shape in local space to the node.
        void SetPositionOffset(Vector3 position);

        /// Set the scale factor to apply to this shape.
        void SetScaleFactor(Vector3 scale);
        void SetScaleFactor(float scale);

        /// Set the rotational offset of the shape in local space to the node.
        void SetRotationOffset(Quaternion rotation);

        /// Get the positional offset of the shape in local space to the node.
        Vector3 GetPositionOffset() const;

        /// Get the scale factor of the shape that is applied on top of node.
        Vector3 GetScaleFactor() const;

        /// Get the rotational offset of the shape in local space to the node.
        Quaternion GetRotationOffset() const;


        /// Set whether the collision size should be effected by the node scale.
        void SetInheritNodeScale(bool enable = true);
        bool GetInheritNodeScale() const;




        /// get local offset matrix.
        Matrix3x4 GetOffsetMatrix();


        Quaternion GetWorldRotation();
        Vector3 GetWorldPosition();

        /// return world transform of collision shape in scene world space
        Matrix3x4 GetWorldTransform();

        /// return physics world transform in physics world space (physics scale applied)
        Matrix3x4 GetPhysicsWorldTransform();

        /// Mark the shape as dirty causing it to be rebuilt by the physics world.
        void MarkDirty(bool dirty = true);
        /// Get the current dirty status.
        bool GetDirty() const;
        /// Get if the CollisionShape is a newton compound (made up of subshapes)
        bool IsCompound() const;

        /// Returns the internal newton collision
        const NewtonCollision* GetNewtonCollision();

        /// Set draw debug geometry from physics world.
        void SetDrawNewtonDebugGeometry(bool enable);
        bool GetDrawNewtonDebugGeometry();

       
        virtual void OnSetEnabled() override;

    protected:

        /// Physics world.
        WeakPtr<PhysicsWorld> physicsWorld_;

        /// Internal Newton Collision
        NewtonCollision* newtonCollision_ = nullptr;

        /// newton Mesh reference
        WeakPtr<NewtonMeshObject> newtonMesh_ = nullptr;


        float staticFriction_ = COLLISION_SHAPE_DEF_STATIC_FRICTION;
        float kineticFriction_ = COLLISION_SHAPE_DEF_KINETIC_FRICTION;
        float elasticity_ = COLLISION_SHAPE_DEF_ELASTICITY;
        float softness_ = COLLISION_SHAPE_DEF_SOFTNESS;

        ///volumetric density.
        float density_ = 1.0f;

        ///is the underlying newtoncollision a compound.
        bool isCompound_ = false;

        /// shape dirty flag
        bool shapeNeedsRebuilt_ = true;

        /// Offset position.
        Vector3 position_;
        /// Scale Factor
        Vector3 scale_ = Vector3::ONE;
        /// Offset rotation.
        Quaternion rotation_;

        ///inherit node scale.
        bool inheritCollisionNodeScales_ = true;

        bool drawPhysicsDebugCollisionGeometry_ = true;

        /// updates the intenal newton collision pointer to reference the appropriate collision instance from the newton cache based on current parameters.
        void updateBuild();
        /// implement this in subclasses to create the internal newton collision
        virtual bool buildNewtonCollision();
        /// Frees the internal collision shape and mesh;
        void freeInternalCollision();

        RigidBody* GetRigidBody();

        void MarkRigidBodyDirty();

        void HandleNodeAdded(StringHash event, VariantMap& eventData);
        void HandleNodeRemoved(StringHash event, VariantMap& eventData);


        virtual void OnNodeSet(Node* node) override;

        virtual void OnNodeSetEnabled(Node* node) override;

    };








}
