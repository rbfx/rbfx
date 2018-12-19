
#include "CollisionShape.h"
#include "PhysicsWorld.h"
#include "RigidBody.h"
#include "NewtonMeshObject.h"


#include "../Core/Context.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Graphics/Model.h"
#include "../IO/Log.h"

#include "Newton.h"
#include "dMatrix.h"

#include "UrhoNewtonConversions.h"
#include "Graphics/Geometry.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/GraphicsDefs.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/StaticModel.h"
#include "Scene/SceneEvents.h"
#include "NewtonDebugDrawing.h"

namespace Urho3D {



    CollisionShape::CollisionShape(Context* context) : Component(context)
    {
        SubscribeToEvent(E_NODEADDED, URHO3D_HANDLER(CollisionShape, HandleNodeAdded));
        SubscribeToEvent(E_NODEREMOVED, URHO3D_HANDLER(CollisionShape, HandleNodeRemoved));
    }

    CollisionShape::~CollisionShape()
    {
        freeInternalCollision();
    }

    void CollisionShape::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(Component);

        URHO3D_ACCESSOR_ATTRIBUTE("Position Offset", GetPositionOffset, SetPositionOffset, Vector3, Vector3::ZERO, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Rotational Offset", GetRotationOffset, SetRotationOffset, Quaternion, Quaternion::IDENTITY, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Scale Factor", GetScaleFactor, SetScaleFactor, Vector3, Vector3::ONE, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Inherit Collision Node Scales", GetInheritNodeScale, SetInheritNodeScale, bool, true, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Static Friction Coefficient", GetStaticFriction, SetStaticFriction, float, COLLISION_SHAPE_DEF_STATIC_FRICTION, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Kinetic Friction Coefficient", GetKineticFriction, SetKineticFriction, float, COLLISION_SHAPE_DEF_KINETIC_FRICTION, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Elasticity", GetElasticity, SetElasticity, float, COLLISION_SHAPE_DEF_ELASTICITY, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Softness", GetSoftness, SetSoftness, float, COLLISION_SHAPE_DEF_SOFTNESS, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Density", GetDensity, SetDensity, float, 1.0f, AM_DEFAULT);
    }




    void CollisionShape::SetFriction(float friction)
    {
        SetStaticFriction(friction);
        SetKineticFriction(friction - 0.1f);
    }

    void CollisionShape::updateBuild()
    {
            // first free any reference to an existing collision.
            freeInternalCollision();

            //call the derived class createNewtonCollision function.
            if (buildNewtonCollision())
            {


                //determine if the built collision is a compound.
                int numSubShapes = 0;
                void* curSubColNode = NewtonCompoundCollisionGetFirstNode(newtonCollision_);
                while (curSubColNode) {
                    numSubShapes++;

                    //set user data on each part.
                    NewtonCollisionSetUserData(NewtonCompoundCollisionGetCollisionFromNode(newtonCollision_, curSubColNode), (void*)this);

                    curSubColNode = NewtonCompoundCollisionGetNextNode(newtonCollision_, curSubColNode);


                }

                if (numSubShapes > 1)
                    isCompound_ = true;
                else
                    isCompound_ = false;


                NewtonCollisionSetUserData(newtonCollision_, (void*)this);
            }
    }

    bool CollisionShape::buildNewtonCollision()
    {
        return true;
    }

    void CollisionShape::freeInternalCollision()
    {
        if (newtonCollision_) {
            physicsWorld_->addToFreeQueue(newtonCollision_);
            newtonCollision_ = nullptr;
        }
    }




    Urho3D::Matrix3x4 CollisionShape::GetOffsetMatrix()
    {
        return Matrix3x4(position_, rotation_, scale_);
    }

    Urho3D::Quaternion CollisionShape::GetWorldRotation()
    {
        return node_->GetWorldRotation() * GetRotationOffset();
    }

    Urho3D::Vector3 CollisionShape::GetWorldPosition()
    {
        return GetWorldTransform().Translation();
    }

    Urho3D::Matrix3x4 CollisionShape::GetWorldTransform()
    {
        if (!node_)
        {
            URHO3D_LOGWARNING("CollisionShape::GetWorldTransform No Node Present");
            return Matrix3x4();

        }

        if(GetInheritNodeScale())
            return node_->GetWorldTransform() * GetOffsetMatrix();
        else
        {
            return Matrix3x4(node_->GetWorldTransform().Translation(), node_->GetWorldTransform().Rotation(), 1.0f) * GetOffsetMatrix();
        }
    }


    Urho3D::Matrix3x4 CollisionShape::GetPhysicsWorldTransform()
    {
        return physicsWorld_->GetPhysicsWorldFrame() * (node_->GetWorldTransform() * GetOffsetMatrix());
    }

    void CollisionShape::SetStaticFriction(float staticFriction)
    {
        if (staticFriction_ != staticFriction)
            staticFriction_ = staticFriction;
    }

    float CollisionShape::GetStaticFriction() const
    {
        return staticFriction_;
    }

    void CollisionShape::SetKineticFriction(float kineticFriction)
    {
        if (kineticFriction_ != kineticFriction)
        {
            kineticFriction_ = kineticFriction;
        }
    }

    float CollisionShape::GetKineticFriction() const
    {
        return kineticFriction_;
    }

    void CollisionShape::SetElasticity(float elasticity)
    {
        if (elasticity_ != elasticity)
        {
            elasticity_ = elasticity;
        }
    }

    float CollisionShape::GetElasticity() const
    {
        return elasticity_;
    }

    void CollisionShape::SetSoftness(float softness)
    {
        if (softness_ != softness)
        {
            softness_ = softness;
        }
    }

    float CollisionShape::GetSoftness() const
    {
        return softness_;
    }

    void CollisionShape::SetDensity(float density)
    {
        if (density != density_)
        {
            density_ = density;
            MarkDirty();
        }
    }

    void CollisionShape::SetPositionOffset(Vector3 position)
    {
        position_ = position; MarkDirty(true);
    }

    void CollisionShape::SetScaleFactor(float scale)
    {
        scale_ = Vector3(scale, scale, scale); MarkDirty(true);
    }

    void CollisionShape::SetScaleFactor(Vector3 scale)
    {
        scale_ = scale; MarkDirty(true);
    }

    void CollisionShape::SetRotationOffset(Quaternion rotation)
    {
        rotation_ = rotation; MarkDirty(true);
    }

    Urho3D::Vector3 CollisionShape::GetPositionOffset() const
    {
        return position_;
    }

    Urho3D::Vector3 CollisionShape::GetScaleFactor() const
    {
        return scale_;
    }

    Urho3D::Quaternion CollisionShape::GetRotationOffset() const
    {
        return rotation_;
    }

    void CollisionShape::SetInheritNodeScale(bool enable /*= true*/)
    {
        if (inheritCollisionNodeScales_ != enable) {
            inheritCollisionNodeScales_ = enable;
            MarkDirty();//we need to rebuild for this change
        }

    }



    bool CollisionShape::GetInheritNodeScale() const
    {
        return inheritCollisionNodeScales_;
    }

    void CollisionShape::MarkDirty(bool dirty /*= true*/)
    {
        if (shapeNeedsRebuilt_ != dirty) {
            shapeNeedsRebuilt_ = dirty;

            // always mark the rigid body dirty as well if the collision shape is dirty.
            if(dirty)
                MarkRigidBodyDirty();
        }
    }

    bool CollisionShape::GetDirty() const
    {
        return shapeNeedsRebuilt_;
    }

    bool CollisionShape::IsCompound() const
    {
        return isCompound_;
    }

    const NewtonCollision* CollisionShape::GetNewtonCollision()
    {
        return newtonCollision_;
    }


    bool CollisionShape::GetDrawNewtonDebugGeometry()
    {
        return drawPhysicsDebugCollisionGeometry_;
    }

    void CollisionShape::SetDrawNewtonDebugGeometry(bool enable)
    {
        drawPhysicsDebugCollisionGeometry_ = enable;
    }



    void CollisionShape::OnSetEnabled()
    {
        MarkRigidBodyDirty();
    }


    RigidBody* CollisionShape::GetRigidBody()
    {
        PODVector<RigidBody*> rootRigidBodies;
        GetRootRigidBodies(rootRigidBodies, node_, true);
        if (rootRigidBodies.Size() > 1)
            return rootRigidBodies[rootRigidBodies.Size() - 2];
        else if (rootRigidBodies.Size() == 1)
        {
            return rootRigidBodies.Back();//return scene rigid body.
        }

        return nullptr;
    }

    void CollisionShape::MarkRigidBodyDirty()
    {
        GetRigidBody()->MarkDirty(true);
    }

    void CollisionShape::OnNodeSet(Node* node)
    {

        if (node)
        {
            ///auto create physics world
            physicsWorld_ = WeakPtr<PhysicsWorld>(GetScene()->GetOrCreateComponent<PhysicsWorld>());


            physicsWorld_->addCollisionShape(this);
            node->AddListener(this);

        }
        else
        {
            freeInternalCollision();
            if (physicsWorld_)
                physicsWorld_->removeCollisionShape(this);

        }

    }



    void CollisionShape::OnNodeSetEnabled(Node* node)
    {
        MarkRigidBodyDirty();
    }

    void CollisionShape::HandleNodeAdded(StringHash event, VariantMap& eventData)
    {
        Node* node = static_cast<Node*>(eventData[NodeAdded::P_NODE].GetPtr());
        Node* newParent = static_cast<Node*>(eventData[NodeRemoved::P_PARENT].GetPtr());

        if (node == node_)
        {
            RebuildPhysicsNodeTree(node);
        }
    }

    void CollisionShape::HandleNodeRemoved(StringHash event, VariantMap& eventData)
    {
        Node* node = static_cast<Node*>(eventData[NodeRemoved::P_NODE].GetPtr());
        if (node == node_)
        {
            Node* oldParent = static_cast<Node*>(eventData[NodeRemoved::P_PARENT].GetPtr());

            if (oldParent)
            {
                RebuildPhysicsNodeTree(oldParent);
            }
            else
            {
                URHO3D_LOGINFO("should not happen");
            }
        }
    }

}
