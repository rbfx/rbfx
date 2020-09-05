//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Core/Context.h"
#include "../Physics/PhysicsWorld.h"
#include "../Physics/PhysicsEvents.h"
#include "../Physics/CollisionShape.h"
#include "../Physics/PhysicsUtils.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../IO/Log.h"

#include <Bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>
#include <Bullet/BulletDynamics/Character/btKinematicCharacterController.h>
#include <cassert>

#include "CharacterController.h"

//=============================================================================
// NOTE! declare these function before #include <Urho3D/DebugNew> 
// otherwise you'll get a compile error for _DEBUG build
//=============================================================================
btPairCachingGhostObject* newPairCachingGhostObj()
{
    return new btPairCachingGhostObject();
}

btKinematicCharacterController* newKinematicCharCtrl(btPairCachingGhostObject *ghostCGO,
                                                     btConvexShape *shape, float stepHeight, const btVector3 &upVec)
{
    return new btKinematicCharacterController(ghostCGO, shape, stepHeight, upVec);
}

#include <Urho3D/DebugNew.h>

//=============================================================================
//=============================================================================
CharacterController::CharacterController(Context* context)
    : Component(context)
{
    pairCachingGhostObject_.reset( newPairCachingGhostObj() );
    pairCachingGhostObject_->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
}

CharacterController::~CharacterController()
{
    ReleaseKinematic();
}

void CharacterController::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterController>();

    URHO3D_ACCESSOR_ATTRIBUTE("Gravity", GetGravity, SetGravity, Vector3, Vector3(0.0f, -14.0f, 0.0f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Collision Layer", int, colLayer_, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Collision Mask", int, colMask_, 0xffff, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Linear Damping", GetLinearDamping, SetLinearDamping, float, 0.2f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Angular Damping", GetAngularDamping, SetAngularDamping, float, 0.2f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Step Height", GetStepHeight, SetStepHeight, float, 0.4f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Jump Height", GetMaxJumpHeight, SetMaxJumpHeight, float, 2.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Fall Speed", GetFallSpeed, SetFallSpeed, float, 55.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Jump Speed", GetJumpSpeed, SetJumpSpeed, float, 9.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Slope", GetMaxSlope, SetMaxSlope, float, 45.0f, AM_DEFAULT);
}

void CharacterController::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    Serializable::OnSetAttribute(attr, src);

    reapplyAttributes_ = true;
}

void CharacterController::ApplyAttributes()
{
    if (reapplyAttributes_)
    {
        ApplySettings(true);
        reapplyAttributes_ = false;
    }
}

void CharacterController::ReleaseKinematic()
{
    if (kinematicController_ != nullptr)
    {
        RemoveKinematicFromWorld();
    }

    kinematicController_.reset();
    pairCachingGhostObject_.reset();
}

void CharacterController::OnNodeSet(Node* node)
{
    if (node)
        node->AddListener(this);
}

void CharacterController::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        if (scene == node_)
            URHO3D_LOGWARNING(GetTypeName() + " should not be created to the root scene node");

        physicsWorld_ = scene->GetComponent<PhysicsWorld>();

        if (physicsWorld_)
        {
            AddKinematicToWorld();
        }
    }
    else
    {
        RemoveKinematicFromWorld();
    }
}

void CharacterController::AddKinematicToWorld()
{
    if (physicsWorld_)
    {
        if (kinematicController_ == nullptr)
        {
            CollisionShape *colShape = GetComponent<CollisionShape>();
            assert(colShape && "missing Collision Shape");

            pairCachingGhostObject_->setCollisionShape(colShape->GetCollisionShape());
            colShapeOffset_ = colShape->GetPosition();

            kinematicController_.reset( newKinematicCharCtrl(pairCachingGhostObject_.get(), 
                                                       dynamic_cast<btConvexShape*>(colShape->GetCollisionShape()),
                                                       stepHeight_, ToBtVector3(Vector3::UP)));
            // apply default settings
            ApplySettings();

            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
            phyicsWorld->addAction(kinematicController_.get());
        }
    }
}

void CharacterController::ApplySettings(bool reapply)
{
    kinematicController_->setGravity(ToBtVector3(gravity_));
    kinematicController_->setLinearDamping(linearDamping_);
    kinematicController_->setAngularDamping(angularDamping_);
    kinematicController_->setStepHeight(stepHeight_);
    kinematicController_->setMaxJumpHeight(maxJumpHeight_);
    kinematicController_->setMaxSlope(M_DEGTORAD * maxSlope_);
    kinematicController_->setJumpSpeed(jumpSpeed_);
    kinematicController_->setFallSpeed(fallSpeed_);

    if (reapply && pairCachingGhostObject_)
    {
        btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
        phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
        phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
    }

    SetTransform(node_->GetWorldPosition(), node_->GetWorldRotation());
}

void CharacterController::RemoveKinematicFromWorld()
{
    if (kinematicController_ && physicsWorld_)
    {
        btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
        phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
        phyicsWorld->removeAction(kinematicController_.get());
    }
}

void CharacterController::SetCollisionLayer(int layer)
{
    if (physicsWorld_)
    {
        if (layer != colLayer_)
        {
            colLayer_ = layer;
            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
        }
    }
}

void CharacterController::SetCollisionMask(int mask)
{
    if (physicsWorld_)
    {
        if (mask != colMask_)
        {
            colMask_ = mask;
            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
        }
    }
}

void CharacterController::SetCollisionLayerAndMask(int layer, int mask)
{
    if (physicsWorld_)
    {
        if (layer != colLayer_ || mask != colMask_)
        {
            colLayer_ = layer;
            colMask_ = mask;
            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
        }
    }
}

const Vector3& CharacterController::GetPosition()
{
    btTransform t = pairCachingGhostObject_->getWorldTransform();
    position_ = ToVector3(t.getOrigin()) - colShapeOffset_;
    return position_;
}

const Quaternion& CharacterController::GetRotation()
{
    btTransform t = pairCachingGhostObject_->getWorldTransform();
    rotation_ = ToQuaternion(t.getRotation());
    return rotation_;
}

void CharacterController::SetTransform(const Vector3& position, const Quaternion& rotation)
{
    btTransform worldTrans;
    worldTrans.setIdentity();
    worldTrans.setRotation(ToBtQuaternion(rotation));
    worldTrans.setOrigin(ToBtVector3(position));
    pairCachingGhostObject_->setWorldTransform(worldTrans);
}

void CharacterController::GetTransform(Vector3& position, Quaternion& rotation)
{
    btTransform worldTrans = pairCachingGhostObject_->getWorldTransform();
    rotation = ToQuaternion(worldTrans.getRotation());
    position = ToVector3(worldTrans.getOrigin());
}

void CharacterController::SetLinearDamping(float linearDamping)
{
    if (linearDamping != linearDamping_)
    {
        linearDamping_ = linearDamping;
        kinematicController_->setLinearDamping(linearDamping_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetAngularDamping(float angularDamping)
{
    if (angularDamping != angularDamping_)
    {
        angularDamping_ = angularDamping;
        kinematicController_->setAngularDamping(angularDamping_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetGravity(const Vector3 &gravity)
{
    if (gravity != gravity_)
    {
        gravity_ = gravity;
        kinematicController_->setGravity(ToBtVector3(gravity_));
        MarkNetworkUpdate();
    }
}

void CharacterController::SetStepHeight(float stepHeight)
{
    if (stepHeight != stepHeight_)
    {
        stepHeight_ = stepHeight;
        kinematicController_->setStepHeight(stepHeight_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetMaxJumpHeight(float maxJumpHeight)
{
    if (maxJumpHeight != maxJumpHeight_)
    {
        maxJumpHeight_ =  maxJumpHeight;
        kinematicController_->setMaxJumpHeight(maxJumpHeight_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetFallSpeed(float fallSpeed)
{
    if (fallSpeed != fallSpeed_)
    {
        fallSpeed_ = fallSpeed;
        kinematicController_->setFallSpeed(fallSpeed_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetJumpSpeed(float jumpSpeed)
{
    if (jumpSpeed != jumpSpeed_)
    {
        jumpSpeed_ = jumpSpeed;
        kinematicController_->setJumpSpeed(jumpSpeed_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetMaxSlope(float maxSlope)
{
    if (maxSlope != maxSlope_)
    {
        maxSlope_ = maxSlope;
        kinematicController_->setMaxSlope(M_DEGTORAD * maxSlope_);
        MarkNetworkUpdate();
    }
}

void CharacterController::SetWalkDirection(const Vector3& walkDir)
{
    kinematicController_->setWalkDirection(ToBtVector3(walkDir));
}

bool CharacterController::OnGround() const
{
    return kinematicController_->onGround();
}

void CharacterController::Jump(const Vector3 &jump)
{
    kinematicController_->jump(ToBtVector3(jump));
}

bool CharacterController::CanJump() const
{
    return kinematicController_->canJump();
}

void CharacterController::ApplyImpulse(const Vector3 &impulse)
{
	kinematicController_->applyImpulse(ToBtVector3(impulse));
}

void CharacterController::SetAngularVelocity(const Vector3 &velocity)
{
	kinematicController_->setAngularVelocity(ToBtVector3(velocity));
}

const Vector3 CharacterController::GetAngularVelocity() const
{
    return ToVector3(kinematicController_->getAngularVelocity());
}

void CharacterController::SetLinearVelocity(const Vector3 &velocity)
{
	kinematicController_->setLinearVelocity(ToBtVector3(velocity));
}

const Vector3 CharacterController::GetLinearVelocity() const
{
    return ToVector3(kinematicController_->getLinearVelocity());
}

void CharacterController::Warp(const Vector3 &position)
{
    kinematicController_->warp(ToBtVector3(position));
}

void CharacterController::DrawDebugGeometry()
{
    kinematicController_->debugDraw(physicsWorld_);
}

