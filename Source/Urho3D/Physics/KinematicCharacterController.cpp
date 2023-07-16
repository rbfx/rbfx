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

#include "../Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Physics/KinematicCharacterController.h"
#include "Urho3D/Physics/PhysicsEvents.h"
#include "Urho3D/Physics/PhysicsUtils.h"
#include "Urho3D/Physics/PhysicsWorld.h"
#include "Urho3D/Scene/Scene.h"

#include <Bullet/BulletDynamics/Character/btKinematicCharacterController.h>
#include <Bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include <cassert>

namespace Urho3D
{

btKinematicCharacterController* newKinematicCharCtrl(btPairCachingGhostObject *ghostCGO,
                                                     btConvexShape *shape, float stepHeight, const btVector3 &upVec)
{
    return new btKinematicCharacterController(ghostCGO, shape, stepHeight, upVec);
}

#include <Urho3D/DebugNew.h>

//=============================================================================
//=============================================================================
KinematicCharacterController::KinematicCharacterController(Context* context)
    : Component(context)
{
    pairCachingGhostObject_ = ea::make_unique<btPairCachingGhostObject>();
    pairCachingGhostObject_->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);

    physicsCollisionData_[PhysicsCollision::P_TRIGGER] = true;
    physicsCollisionData_[PhysicsCollision::P_CONTACTS] = VariantVector{};

    nodeCollisionData_[NodeCollision::P_TRIGGER] = true;
    nodeCollisionData_[NodeCollision::P_CONTACTS] = VariantVector{};
}

KinematicCharacterController::~KinematicCharacterController()
{
    ReleaseKinematic();
}

void KinematicCharacterController::RegisterObject(Context* context)
{
    context->AddFactoryReflection<KinematicCharacterController>(Category_Physics);

    URHO3D_ACCESSOR_ATTRIBUTE("Gravity", GetGravity, SetGravity, Vector3, Vector3(0.0f, -14.0f, 0.0f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Collision Layer", int, colLayer_, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Collision Mask", int, colMask_, 0xffff, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Linear Damping", GetLinearDamping, SetLinearDamping, float, 0.2f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Angular Damping", GetAngularDamping, SetAngularDamping, float, 0.2f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Height", GetHeight, SetHeight, float, 1.8f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Diameter", GetDiameter, SetDiameter, float, 0.7f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Offset", GetOffset, SetOffset, Vector3, Vector3(0.0f, 0.9f, 0.0f), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Step Height", GetStepHeight, SetStepHeight, float, 0.4f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Jump Height", GetMaxJumpHeight, SetMaxJumpHeight, float, 2.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Fall Speed", GetFallSpeed, SetFallSpeed, float, 55.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Jump Speed", GetJumpSpeed, SetJumpSpeed, float, 9.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Slope", GetMaxSlope, SetMaxSlope, float, 45.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Activate Triggers", bool, activateTriggers_, true, AM_DEFAULT);
}

void KinematicCharacterController::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    Serializable::OnSetAttribute(attr, src);

    readdToWorld_ = true;
}

void KinematicCharacterController::ApplyAttributes()
{
    AddKinematicToWorld();
    if (readdToWorld_)
    {
        ApplySettings(true);
        readdToWorld_ = false;
    }
}

void KinematicCharacterController::ReleaseKinematic()
{
    if (kinematicController_ != nullptr)
    {
        RemoveKinematicFromWorld();
    }

    kinematicController_.reset();
    pairCachingGhostObject_.reset();
}

void KinematicCharacterController::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
        node_->AddListener(this);
}

void KinematicCharacterController::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        if (scene == node_)
            URHO3D_LOGWARNING(GetTypeName() + " should not be created to the root scene node");

        physicsWorld_ = scene->GetOrCreateComponent<PhysicsWorld>();

        if (physicsWorld_)
        {
            AddKinematicToWorld();
        }
        SubscribeToEvent(physicsWorld_, E_PHYSICSPREUPDATE, URHO3D_HANDLER(KinematicCharacterController, HandlePhysicsPreUpdate));
        SubscribeToEvent(physicsWorld_, E_PHYSICSPOSTSTEP, URHO3D_HANDLER(KinematicCharacterController, HandlePhysicsPostStep));
        SubscribeToEvent(physicsWorld_, E_PHYSICSPOSTUPDATE, URHO3D_HANDLER(KinematicCharacterController, HandlePhysicsPostUpdate));
    }
    else
    {
        RemoveKinematicFromWorld();
        UnsubscribeFromEvent(physicsWorld_, E_PHYSICSPREUPDATE);
        UnsubscribeFromEvent(physicsWorld_, E_PHYSICSPOSTSTEP);
        UnsubscribeFromEvent(physicsWorld_, E_PHYSICSPOSTUPDATE);
    }
}

void KinematicCharacterController::HandlePhysicsPreUpdate(StringHash eventType, VariantMap& eventData)
{
    const Vector3 position = node_->GetWorldPosition();
    if (!position.Equals(latestPosition_, M_LARGE_EPSILON))
    {
        WarpKinematic(position);
    }
}

void KinematicCharacterController::HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData)
{
    previousPosition_ = nextPosition_;
    nextPosition_ = GetRawPosition();
}

void KinematicCharacterController::HandlePhysicsPostUpdate(StringHash eventType, VariantMap& eventData)
{
    ActivateTriggers();

    if (physicsWorld_ && physicsWorld_->GetInterpolation())
    {
        const float timeStep = eventData[PhysicsPostUpdate::P_TIMESTEP].GetFloat();
        const float overtime = eventData[PhysicsPostUpdate::P_OVERTIME].GetFloat();
        const float updateFrequency = physicsWorld_->GetFps();
        positionOffset_ = Lerp(positionOffset_, Vector3::ZERO, ExpSmoothing(smoothingConstant_, timeStep));
        latestPosition_ = Lerp(previousPosition_, nextPosition_, overtime * updateFrequency) + positionOffset_;
    }
    else
    {
        latestPosition_ = nextPosition_;
    }

    node_->SetWorldPosition(latestPosition_);
}

void KinematicCharacterController::SendCollisionEvent(StringHash physicsEvent, StringHash nodeEvent,
    RigidBody* otherBody)
{
    if (!otherBody)
        return;

    {
        using namespace PhysicsCollision;
        physicsCollisionData_[P_NODEB] = otherBody->GetNode();
        physicsCollisionData_[P_BODYB] = otherBody;
        SendEvent(physicsEvent, physicsCollisionData_);
    }

    if (GetNode())
    {
        using namespace NodeCollision;
        nodeCollisionData_[P_BODY].Clear();
        nodeCollisionData_[P_OTHERNODE] = otherBody->GetNode();
        nodeCollisionData_[P_OTHERBODY] = otherBody;
        GetNode()->SendEvent(nodeEvent, nodeCollisionData_);
    }
    if (otherBody->GetNode())
    {
        using namespace NodeCollision;
        nodeCollisionData_[P_BODY] = otherBody;
        nodeCollisionData_[P_OTHERNODE] = GetNode();
        nodeCollisionData_[P_OTHERBODY].Clear();
        otherBody->GetNode()->SendEvent(nodeEvent, nodeCollisionData_);
    }
}

void KinematicCharacterController::ActivateTriggers()
{
    if (!activateTriggers_)
        return;

    physicsCollisionData_[PhysicsCollision::P_NODEA] = GetNode();
    physicsCollisionData_[PhysicsCollision::P_BODYA] = static_cast<RigidBody*>(nullptr);

    activeTriggerFlag_ = !activeTriggerFlag_;
    const int num = kinematicController_->getGhostObject()->getNumOverlappingObjects();
    for (int i = 0; i < num; ++i)
    {
        if (const auto* other = kinematicController_->getGhostObject()->getOverlappingObject(i))
        {
            // Send event when touching trigger
            if (other->getUserPointer() && !other->hasContactResponse())
            {
                SharedPtr<RigidBody> body{static_cast<RigidBody*>(other->getUserPointer())};
                if (body && body->GetNode())
                {
                    auto itPair = activeTriggerContacts_.emplace(body, activeTriggerFlag_);
                    if (!itPair.second)
                    {
                        itPair.first->second = activeTriggerFlag_;

                        SendCollisionEvent(E_PHYSICSCOLLISION, E_NODECOLLISION, body);
                    }
                    else
                    {
                        SendCollisionEvent(E_PHYSICSCOLLISIONSTART, E_NODECOLLISIONSTART, body);
                    }
                }
            }
        }
    }

    for (auto itKV = activeTriggerContacts_.begin(), last = activeTriggerContacts_.end(); itKV != last;)
    {
        if (itKV->second != activeTriggerFlag_)
        {
            SendCollisionEvent(E_PHYSICSCOLLISIONEND, E_NODECOLLISIONEND, itKV->first);

            itKV = activeTriggerContacts_.erase(itKV);
        }
        else
        {
            ++itKV;
        }
    }
}

btCapsuleShape* KinematicCharacterController::GetOrCreateShape()
{
    if (!shape_)
    {
        ResetShape();
    }
    return shape_.get();
}

void KinematicCharacterController::ResetShape()
{
    shape_ = ea::make_unique<btCapsuleShape>(diameter_ * 0.5f, Max(height_ - diameter_, 0.0f));
    if (pairCachingGhostObject_)
    {
        pairCachingGhostObject_->setCollisionShape(shape_.get());
    }
    if (kinematicController_)
    {
        kinematicController_->setCollisionShape(shape_.get());
    }
}

void KinematicCharacterController::AddKinematicToWorld()
{
    if (physicsWorld_)
    {
        if (kinematicController_ == nullptr)
        {
            btCapsuleShape* btColShape = GetOrCreateShape();
            pairCachingGhostObject_->setCollisionShape(btColShape);

            kinematicController_ = ea::make_unique<btKinematicCharacterController>(pairCachingGhostObject_.get(),
                                                       btColShape,
                                                       stepHeight_, ToBtVector3(Vector3::UP));
            // apply default settings
            ApplySettings(false);

            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
            phyicsWorld->addAction(kinematicController_.get());
        }
    }
}

void KinematicCharacterController::ApplySettings(bool readdToWorld)
{
    kinematicController_->setGravity(ToBtVector3(gravity_));
    kinematicController_->setLinearDamping(linearDamping_);
    kinematicController_->setAngularDamping(angularDamping_);
    kinematicController_->setStepHeight(stepHeight_);
    kinematicController_->setMaxJumpHeight(maxJumpHeight_);
    kinematicController_->setMaxSlope(M_DEGTORAD * maxSlope_);
    kinematicController_->setJumpSpeed(jumpSpeed_);
    kinematicController_->setFallSpeed(fallSpeed_);

    if (readdToWorld && pairCachingGhostObject_)
    {
        btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
        phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
        phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), colLayer_, colMask_);
    }

    WarpKinematic(node_->GetWorldPosition());
}

void KinematicCharacterController::RemoveKinematicFromWorld()
{
    if (kinematicController_ && physicsWorld_)
    {
        btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
        phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
        phyicsWorld->removeAction(kinematicController_.get());
    }
}

void KinematicCharacterController::SetCollisionLayer(unsigned layer)
{
    if (physicsWorld_)
    {
        if (layer != colLayer_)
        {
            colLayer_ = layer;
            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), static_cast<int>(colLayer_), static_cast<int>(colMask_));
        }
    }
}

void KinematicCharacterController::SetCollisionMask(unsigned mask)
{
    if (physicsWorld_)
    {
        if (mask != colMask_)
        {
            colMask_ = mask;
            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), static_cast<int>(colLayer_), static_cast<int>(colMask_));
        }
    }
}

void KinematicCharacterController::SetCollisionLayerAndMask(unsigned layer, unsigned mask)
{
    if (physicsWorld_)
    {
        if (layer != colLayer_ || mask != colMask_)
        {
            colLayer_ = layer;
            colMask_ = mask;
            btDiscreteDynamicsWorld *phyicsWorld = physicsWorld_->GetWorld();
            phyicsWorld->removeCollisionObject(pairCachingGhostObject_.get());
            phyicsWorld->addCollisionObject(pairCachingGhostObject_.get(), static_cast<int>(colLayer_), static_cast<int>(colMask_));
        }
    }
}

Vector3 KinematicCharacterController::GetRawPosition() const
{
    btTransform t = pairCachingGhostObject_->getWorldTransform();
    return ToVector3(t.getOrigin()) - colShapeOffset_;
}

void KinematicCharacterController::AdjustRawPosition(const Vector3& offset, float smoothConstant)
{
    smoothingConstant_ = smoothConstant;
    positionOffset_ -= offset;
    WarpKinematic(GetRawPosition() + offset);
    node_->SetWorldPosition(latestPosition_);
}

void KinematicCharacterController::WarpKinematic(const Vector3& position)
{
    latestPosition_ = position + positionOffset_;
    previousPosition_ = position;
    nextPosition_ = position;

    const btVector3 objectPosition = ToBtVector3(position + colShapeOffset_);
    pairCachingGhostObject_->setWorldTransform(btTransform(btQuaternion::getIdentity(), objectPosition));
}

void KinematicCharacterController::SetLinearDamping(float linearDamping)
{
    if (linearDamping != linearDamping_)
    {
        linearDamping_ = linearDamping;
        kinematicController_->setLinearDamping(linearDamping_);
    }
}

void KinematicCharacterController::SetAngularDamping(float angularDamping)
{
    if (angularDamping != angularDamping_)
    {
        angularDamping_ = angularDamping;
        kinematicController_->setAngularDamping(angularDamping_);
    }
}

void KinematicCharacterController::SetGravity(const Vector3 &gravity)
{
    if (gravity != gravity_)
    {
        gravity_ = gravity;
        kinematicController_->setGravity(ToBtVector3(gravity_));
    }
}

void KinematicCharacterController::SetHeight(float height)
{
    if (height != height_)
    {
        height_ = height;
        ResetShape();
    }
}

void KinematicCharacterController::SetDiameter(float diameter)
{
    if (diameter != diameter_)
    {
        diameter_ = diameter;
        ResetShape();
    }
}

void KinematicCharacterController::SetActivateTriggers(bool activateTriggers)
{
    activateTriggers_ = activateTriggers;
}

void KinematicCharacterController::SetOffset(const Vector3& offset)
{
    if (offset != colShapeOffset_)
    {
        auto diff = offset - colShapeOffset_;
        colShapeOffset_ = offset;
        if (pairCachingGhostObject_)
        {
            auto transform = pairCachingGhostObject_->getWorldTransform();
            transform.setOrigin(transform.getOrigin() + ToBtVector3(diff));
            pairCachingGhostObject_->setWorldTransform(transform);
        }
    }
}

void KinematicCharacterController::SetStepHeight(float stepHeight)
{
    if (stepHeight != stepHeight_)
    {
        stepHeight_ = stepHeight;
        kinematicController_->setStepHeight(stepHeight_);
    }
}

void KinematicCharacterController::SetMaxJumpHeight(float maxJumpHeight)
{
    if (maxJumpHeight != maxJumpHeight_)
    {
        maxJumpHeight_ =  maxJumpHeight;
        kinematicController_->setMaxJumpHeight(maxJumpHeight_);
    }
}

void KinematicCharacterController::SetFallSpeed(float fallSpeed)
{
    if (fallSpeed != fallSpeed_)
    {
        fallSpeed_ = fallSpeed;
        kinematicController_->setFallSpeed(fallSpeed_);
    }
}

void KinematicCharacterController::SetJumpSpeed(float jumpSpeed)
{
    if (jumpSpeed != jumpSpeed_)
    {
        jumpSpeed_ = jumpSpeed;
        kinematicController_->setJumpSpeed(jumpSpeed_);
    }
}

void KinematicCharacterController::SetMaxSlope(float maxSlope)
{
    if (maxSlope != maxSlope_)
    {
        maxSlope_ = maxSlope;
        kinematicController_->setMaxSlope(M_DEGTORAD * maxSlope_);
    }
}

void KinematicCharacterController::SetWalkIncrement(const Vector3& walkDir)
{
    kinematicController_->setWalkDirection(ToBtVector3(walkDir));
}

bool KinematicCharacterController::OnGround() const
{
    return kinematicController_->onGround();
}

void KinematicCharacterController::Jump(const Vector3 &jump)
{
    kinematicController_->jump(ToBtVector3(jump));
}

bool KinematicCharacterController::CanJump() const
{
    return kinematicController_->canJump();
}

void KinematicCharacterController::ApplyImpulse(const Vector3 &impulse)
{
	kinematicController_->applyImpulse(ToBtVector3(impulse));
}

void KinematicCharacterController::SetAngularVelocity(const Vector3 &velocity)
{
	kinematicController_->setAngularVelocity(ToBtVector3(velocity));
}

const Vector3 KinematicCharacterController::GetAngularVelocity() const
{
    return ToVector3(kinematicController_->getAngularVelocity());
}

void KinematicCharacterController::SetLinearVelocity(const Vector3 &velocity)
{
	kinematicController_->setLinearVelocity(ToBtVector3(velocity));
}

const Vector3 KinematicCharacterController::GetLinearVelocity() const
{
    return ToVector3(kinematicController_->getLinearVelocity());
}

void KinematicCharacterController::DrawDebugGeometry()
{
    kinematicController_->debugDraw(physicsWorld_);
}

}
