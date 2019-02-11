//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Physics/PhysicsWorld.h"
#include "../Physics/CollisionShape.h"
#include "../Physics/RigidBody.h"
#include "../Core/Context.h"
#include "../Graphics/Model.h"
#include "IO/Log.h"
#include "Scene/Scene.h"
#include "Scene/Node.h"
#include "dMatrix.h"
#include "Newton.h"
#include "NewtonDebugDrawing.h"
#include "UrhoNewtonConversions.h"
#include "Scene/SceneEvents.h"
#include "Engine/Engine.h"
#include "Core/Profiler.h"
#include "Core/Object.h"
#include "dQuaternion.h"
#include "Constraint.h"
#include "Resource/ResourceCache.h"
#include "Graphics/DebugRenderer.h"
#include "dgQuaternion.h"


namespace Urho3D {






    RigidBody::RigidBody(Context* context) : Component(context)
    {
        SubscribeToEvent(E_NODEADDED, URHO3D_HANDLER(RigidBody, HandleNodeAdded));
        SubscribeToEvent(E_NODEREMOVED, URHO3D_HANDLER(RigidBody, HandleNodeRemoved));
    }

    RigidBody::~RigidBody()
    {
        if (nextAngularVelocityNeeded_ || nextImpulseNeeded_ || nextLinearVelocityNeeded_ || nextSleepStateNeeded_)
            URHO3D_LOGWARNING("Rigid Body Scheduled update did not get a chance to apply!  Consider saving the updates as attributes.");

        
    }

    void RigidBody::RegisterObject(Context* context)
    {
        context->RegisterFactory<RigidBody>(DEF_PHYSICS_CATEGORY.CString());

        URHO3D_COPY_BASE_ATTRIBUTES(Component);

        
        URHO3D_ACCESSOR_ATTRIBUTE("MassScale", GetMassScale, SetMassScale, float, 1.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Linear Velocity", GetLinearVelocity, SetLinearVelocityHard, Vector3, Vector3::ZERO, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Angular Velocity", GetAngularVelocity, SetAngularVelocity, Vector3, Vector3::ZERO, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Continuous Collision", GetContinuousCollision, SetContinuousCollision, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Linear Damping", GetLinearDamping, SetLinearDamping, float, 0.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Angular Damping", GetAngularDamping, SetAngularDamping, float, 0.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Interpolation Factor", GetInterpolationFactor, SetInterpolationFactor, float, 1.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Trigger Mode", GetTriggerMode, SetTriggerMode, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Collision Layer", GetCollisionLayer, SetCollisionLayer, unsigned, DEFAULT_COLLISION_LAYER, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Collision Mask", GetCollisionLayerMask, SetCollisionLayerMask, unsigned, DEFAULT_COLLISION_MASK, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("No Collide Override", GetNoCollideOverride, SetNoCollideOverride, bool, false, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Collision Body Exceptions", VariantMap, collisionExceptions_, VariantMap(), AM_DEFAULT | AM_NOEDIT);
        URHO3D_ATTRIBUTE("Generate Contacts", bool, generateContacts_, true, AM_DEFAULT);


        URHO3D_ATTRIBUTE("Net Force", Vector3, netForce_, Vector3::ZERO, AM_DEFAULT | AM_NOEDIT);
        URHO3D_ATTRIBUTE("Net Torque", Vector3, netTorque_, Vector3::ZERO, AM_DEFAULT | AM_NOEDIT);
        URHO3D_ATTRIBUTE("Is Scene Root Body", bool, sceneRootBodyMode_, false, AM_DEFAULT | AM_NOEDIT);

       

    }


    void RigidBody::SetMassScale(float massDensityScale)
    {
        if (massScale_ != massDensityScale) {
            massScale_ = massDensityScale;
            MarkDirty(true);
        }
    }
   

    void RigidBody::SetWorldTransform(const Matrix3x4& transform)
    {
        if (newtonBody_)
        {
            Activate();

            Matrix3x4 scaleLessTransform(transform.Translation(), transform.Rotation(), 1.0f);
            NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(scaleLessTransform))[0][0]);
        }
        else
        {

            nextTransformNeeded_ = true;
            nextTransform_ = transform;
        }
    }

    void RigidBody::SetWorldPosition(const Vector3& position)
    {
        if (newtonBody_)
        {
            Activate();


            dgQuaternion orientation;
            NewtonBodyGetRotation(newtonBody_, &orientation.m_x);

            Matrix3x4 transform(position, NewtonToUrhoQuat(orientation), 1.0f);
            NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(transform))[0][0]);
        }
        else
        {

            nextPositionNeeded_ = true;
            nextPosition_ = position;
        }
    }

    void RigidBody::SetWorldRotation(const Quaternion& quaternion)
    {
        if (newtonBody_)
        {
            Activate();

            dVector pos;
            NewtonBodyGetPosition(newtonBody_, &pos[0]);

            Matrix3x4 transform(NewtonToUrhoVec3(pos), quaternion, 1.0f);
            NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(transform))[0][0]);
        }
        else
        {

            nextOrientationNeeded_ = true;
            nextOrientation_ = quaternion;
        }
    }

    Urho3D::Matrix3x4 RigidBody::GetPhysicsTransform(bool scaledPhysicsWorldFrame)
    {
        
        if(newtonBody_){
            dMatrix bodyMatrix;
            NewtonBodyGetMatrix(newtonBody_, &bodyMatrix[0][0]);
            
            if(scaledPhysicsWorldFrame)
                return physicsWorld_->SceneToPhysics_Domain(Matrix3x4(NewtonToUrhoMat4(bodyMatrix)));
            else
                return Matrix3x4(NewtonToUrhoMat4(bodyMatrix));
        
        }
        else {
            if(scaledPhysicsWorldFrame)
                return Matrix3x4(physicsWorld_->SceneToPhysics_Domain(targetNodePos_), targetNodeRotation_, 1.0f);
            else
                return Matrix3x4(targetNodePos_, targetNodeRotation_, 1.0f);
            
        }
    }

    Urho3D::Vector3 RigidBody::GetPhysicsPosition(bool scaledPhysicsWorldFrame /*= false*/)
    {
                if(newtonBody_){
                    dVector bodyPos;
                    NewtonBodyGetPosition(newtonBody_, &bodyPos[0]);
                    if (scaledPhysicsWorldFrame)
                        return physicsWorld_->SceneToPhysics_Domain(NewtonToUrhoVec3(bodyPos));
                    else
                        return NewtonToUrhoVec3(bodyPos);
                }
                else {
                    if (scaledPhysicsWorldFrame)
                        return physicsWorld_->SceneToPhysics_Domain(targetNodePos_);
                    else
                        return targetNodePos_;
                    
                }
        

    }
    
    Quaternion RigidBody::GetPhysicsRotation() { 
        if(newtonBody_){
            dgQuaternion bodyOrientation;
            NewtonBodyGetRotation(newtonBody_, &bodyOrientation.m_x);
            return NewtonToUrhoQuat(bodyOrientation);
        }
        else {
            return targetNodeRotation_; 
        }
    }
    
    Vector3 RigidBody::GetCenterOfMassPosition(bool scaledPhysicsWorldFrame)
    {
        if(newtonBody_)
        {
            dVector comPosition;
            NewtonBodyGetCentreOfMass(newtonBody_, &comPosition[0]);
            if(scaledPhysicsWorldFrame)
            {
                return physicsWorld_->SceneToPhysics_Domain(NewtonToUrhoVec3(comPosition));
            }
            else {
                return NewtonToUrhoVec3(comPosition);
            }
        }
        else {
            return Vector3::ZERO;
        }
    }
    
    Matrix3x4 RigidBody::GetCenterOfMassTransform(bool scaledPhysicsWorldFrame)
    {
        if(newtonBody_)
        {
            return Matrix3x4(GetCenterOfMassPosition(scaledPhysicsWorldFrame), GetPhysicsRotation(), 1.0f);
        }
        else {
            return Matrix3x4::IDENTITY;
        }
    }
    
    
    
    
    
    void RigidBody::SetLinearVelocity(const Vector3& worldVelocity, bool useForces)
    {
        if (newtonBody_)
        {
            Activate();
            if (useForces)
            {
                dVector curWorldVel;
                NewtonBodyGetVelocity(newtonBody_, &curWorldVel[0]);

                dVector worldVel = UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(worldVelocity)) - curWorldVel;
                dVector bodyWorldPos;
                NewtonBodyGetPosition(newtonBody_, &bodyWorldPos[0]);
                NewtonBodyAddImpulse(newtonBody_, &worldVel[0], &bodyWorldPos[0], physicsWorld_->timeStepTarget_*GetScene()->GetTimeScale());
            }
            else
                NewtonBodySetVelocity(newtonBody_, &UrhoToNewton(nextLinearVelocity_)[0]);
        }
        else
        {
            nextLinearVelocity_ = physicsWorld_->SceneToPhysics_Domain(worldVelocity);
            nextLinearVelocityUseForces_ = useForces;
            nextLinearVelocityNeeded_ = true;
        }
    }


    void RigidBody::SetLinearVelocityHard(const Vector3& worldVelocity)
    {
        SetLinearVelocity(worldVelocity, false);
    }

    void RigidBody::SetAngularVelocity(const Vector3& angularVelocity)
    {
        if (newtonBody_)
        {
            Activate();
            NewtonBodySetOmega(newtonBody_, &UrhoToNewton(nextAngularVelocity_)[0]);
        }
        else
        {
            nextAngularVelocity_ = physicsWorld_->SceneToPhysics_Domain(angularVelocity);
            nextAngularVelocityNeeded_ = true;
        }
    }

    void RigidBody::SetLinearDamping(float dampingFactor)
    {
        dampingFactor = Urho3D::Clamp<float>(dampingFactor, 0.0f, dampingFactor);

        if (linearDampening_ != dampingFactor) {
            linearDampening_ = dampingFactor;
        }
    }

    void RigidBody::SetAngularDamping(float angularDamping)
    {
        angularDamping = Urho3D::Clamp(angularDamping, 0.0f, angularDamping);

        if (angularDamping != angularDamping) {
            angularDampening_ = angularDamping;
        }
    }

    void RigidBody::SetInternalLinearDamping(float damping)
    {
        if (linearDampeningInternal_ != damping) {
            linearDampeningInternal_ = damping;

            if (newtonBody_)
            {
                NewtonBodySetLinearDamping(newtonBody_, linearDampeningInternal_);
            }
            else
            {
                MarkDirty();
            }
        }
    }

    void RigidBody::SetInternalAngularDamping(float angularDamping)
    {
        angularDampeningInternal_ = Vector3(angularDamping, angularDamping, angularDamping);
        if (newtonBody_)
        {
            NewtonBodySetAngularDamping(newtonBody_, &UrhoToNewton(angularDampeningInternal_)[0]);
        }
        else
        {
            MarkDirty();
        }
    }




    void RigidBody::SetInterpolationFactor(float factor /*= 0.0f*/)
    {
        interpolationFactor_ = Clamp(factor, M_EPSILON, 1.0f);
    }



    void RigidBody::SetContinuousCollision(bool sweptCollision)
    {
        if (continuousCollision_ != sweptCollision) {
            continuousCollision_ = sweptCollision;
            if (newtonBody_) {
                NewtonBodySetContinuousCollisionMode(newtonBody_, sweptCollision);
            }
        }
    }


    void RigidBody::SetAutoSleep(bool enableAutoSleep)
    {
        if (autoSleep_ != enableAutoSleep)
        {
            autoSleep_ = enableAutoSleep;
            if (newtonBody_)
            {
                NewtonBodySetAutoSleep(newtonBody_, autoSleep_);
            }
        }
    }

    void RigidBody::Activate()
    {
        if (newtonBody_)
        {
            NewtonBodySetSleepState(newtonBody_, false);
        }
        else
        {
            nextSleepStateNeeded_ = true;
            nextSleepState_ = false;
        }
    }

    void RigidBody::DeActivate()
    {
        if (newtonBody_)
        {
            NewtonBodySetSleepState(newtonBody_, true);
        }
        else
        {
            nextSleepStateNeeded_ = true;
            nextSleepState_ = true;
        }
    }

    void RigidBody::SetIsSceneRootBody(bool enable)
    {
        if (sceneRootBodyMode_ != enable) {
            sceneRootBodyMode_ = enable;
            MarkDirty(true);
        }
    }

    void RigidBody::DrawDebugGeometry(DebugRenderer* debug, bool depthTest, bool showAABB /*= true*/, bool showCollisionMesh /*= true*/, bool showCenterOfMass /*= true*/, bool showContactForces /*= true*/)
    {
        Component::DrawDebugGeometry(debug, depthTest);
        if (newtonBody_ && GetEffectiveNewtonCollision()) {
            if (showAABB )
            {
                    dMatrix matrix;
                    dVector p0(0.0f);
                    dVector p1(0.0f);

                    NewtonBodyGetMatrix(newtonBody_, &matrix[0][0]);
                    NewtonCollisionCalculateAABB(GetEffectiveNewtonCollision(), &matrix[0][0], &p0[0], &p1[0]);


                    Vector3 min = physicsWorld_->PhysicsToScene_Domain(NewtonToUrhoVec3(p0));
                    Vector3 max = physicsWorld_->PhysicsToScene_Domain(NewtonToUrhoVec3(p1));
                    BoundingBox box(min, max);
                    debug->AddBoundingBox(box, Color::YELLOW, depthTest, false);

            }
            if (showCollisionMesh) NewtonDebug_BodyDrawCollision(physicsWorld_, newtonBody_, debug, depthTest);
            if (showCenterOfMass) {


                dMatrix matrix;
                dVector com(0.0f);
                dVector p0(0.0f);
                dVector p1(0.0f);

                NewtonCollision* collision = GetEffectiveNewtonCollision();
                NewtonBodyGetCentreOfMass(newtonBody_, &com[0]);
                NewtonBodyGetMatrix(newtonBody_, &matrix[0][0]);
                NewtonCollisionCalculateAABB(collision, &matrix[0][0], &p0[0], &p1[0]);

                Vector3 aabbMin = NewtonToUrhoVec3(p0);
                Vector3 aabbMax = NewtonToUrhoVec3(p1);
                float aabbSize = (aabbMax - aabbMin).Length()*0.1f;

                dVector o(matrix.TransformVector(com));

                dVector x(o + matrix.RotateVector(dVector(1.0f, 0.0f, 0.0f, 0.0f))*aabbSize);
                debug->AddLine(physicsWorld_->PhysicsToScene_Domain(Vector3((o.m_x), (o.m_y), (o.m_z))), physicsWorld_->PhysicsToScene_Domain(Vector3((x.m_x), (x.m_y), (x.m_z))), Color::RED, depthTest);


                dVector y(o + matrix.RotateVector(dVector(0.0f, 1.0f, 0.0f, 0.0f))*aabbSize);
                debug->AddLine(physicsWorld_->PhysicsToScene_Domain(Vector3((o.m_x), (o.m_y), (o.m_z))), physicsWorld_->PhysicsToScene_Domain(Vector3((y.m_x), (y.m_y), (y.m_z))), Color::GREEN, depthTest);


                dVector z(o + matrix.RotateVector(dVector(0.0f, 0.0f, 1.0f, 0.0f))*aabbSize);
                debug->AddLine(physicsWorld_->PhysicsToScene_Domain(Vector3((o.m_x), (o.m_y), (o.m_z))), physicsWorld_->PhysicsToScene_Domain(Vector3((z.m_x), (z.m_y), (z.m_z))), Color::BLUE, depthTest);



            }
            if (showContactForces)
            {

                dFloat mass;
                dFloat Ixx;
                dFloat Iyy;
                dFloat Izz;
                NewtonBodyGetMass(newtonBody_, &mass, &Ixx, &Iyy, &Izz);

                //draw normal forces in term of acceleration.
                //this mean that two bodies with same shape but different mass will display the same force
                if (mass > 0.0f) {
                    float scaleFactor = 0.1f / mass;
                    for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(newtonBody_); joint; joint = NewtonBodyGetNextContactJoint(newtonBody_, joint)) {
                        if (NewtonJointIsActive(joint)) {
                            for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
                                dVector point(0.0f);
                                dVector normal(0.0f);
                                dVector tangentDir0(0.0f);
                                dVector tangentDir1(0.0f);
                                dVector contactForce(0.0f);
                                NewtonMaterial* const material = NewtonContactGetMaterial(contact);

                                NewtonMaterialGetContactForce(material, newtonBody_, &contactForce.m_x);
                                NewtonMaterialGetContactPositionAndNormal(material, newtonBody_, &point.m_x, &normal.m_x);
                                dVector normalforce(normal.Scale(contactForce.DotProduct3(normal)));
                                dVector p0(point);
                                dVector p1(point + normalforce.Scale(scaleFactor));

                                debug->AddLine(physicsWorld_->PhysicsToScene_Domain(Vector3((p0.m_x), (p0.m_y), (p0.m_z))), physicsWorld_->PhysicsToScene_Domain(Vector3((p1.m_x), (p1.m_y), (p1.m_z))), Color::GRAY, depthTest);



                                // these are the components of the tangents forces at the contact point, the can be display at the contact position point.
                                NewtonMaterialGetContactTangentDirections(material, newtonBody_, &tangentDir0[0], &tangentDir1[0]);
                                dVector tangentForce1(tangentDir0.Scale((contactForce.DotProduct3(tangentDir0)) * scaleFactor));
                                dVector tangentForce2(tangentDir1.Scale((contactForce.DotProduct3(tangentDir1)) * scaleFactor));

                                p1 = point + tangentForce1.Scale(scaleFactor);
                                debug->AddLine(physicsWorld_->PhysicsToScene_Domain(Vector3((p0.m_x), (p0.m_y), (p0.m_z))), physicsWorld_->PhysicsToScene_Domain(Vector3((p1.m_x), (p1.m_y), (p1.m_z))), Color::GRAY, depthTest);


                                p1 = point + tangentForce2.Scale(scaleFactor);
                                debug->AddLine(physicsWorld_->PhysicsToScene_Domain(Vector3((p0.m_x), (p0.m_y), (p0.m_z))), physicsWorld_->PhysicsToScene_Domain(Vector3((p1.m_x), (p1.m_y), (p1.m_z))), Color::GRAY, depthTest);
                            }
                        }
                    }
                }
            }
        }
    }


    void RigidBody::MarkDirty(bool dirty)
    {
        needsRebuilt_ = dirty;

    }

    void RigidBody::MarkInternalTransformDirty(bool dirty)
    {
        transformDirty_ = dirty;
    }

    bool RigidBody::GetInternalTransformDirty()
    {
        return transformDirty_;
    }


    void RigidBody::OnSetEnabled()
    {
        if (IsEnabledEffective()) {
            MarkDirty(true);//rebuild
        }
        else
        {
            freeBody();
        }
    }

    Urho3D::RigidBodyContactEntry* RigidBody::GetCreateContactEntry(RigidBody* otherBody)
    {
        //look through existing
        RigidBodyContactEntry* entry = contactEntries_[otherBody->GetID()];

        if (!entry)
        {


            //find a good one to grab from the physics world pool - if none available - grow the pool.
            int startingIdx = physicsWorld_->contactEntryPoolCurIdx_;
            while (!physicsWorld_->contactEntryPool_[physicsWorld_->contactEntryPoolCurIdx_]->expired_) {

                physicsWorld_->contactEntryPoolCurIdx_++;

                if (physicsWorld_->contactEntryPoolCurIdx_ > physicsWorld_->contactEntryPool_.Size() - 1) {
                    physicsWorld_->contactEntryPoolCurIdx_ = 0;
                }
                if (physicsWorld_->contactEntryPoolCurIdx_ == startingIdx)
                {

                    //grow the pool
                    int prevSize = physicsWorld_->contactEntryPool_.Size();
                    for (int i = 0; i < physicsWorld_->contactEntryPoolSize_; i++) {
                        physicsWorld_->contactEntryPool_.Insert(physicsWorld_->contactEntryPool_.Size(), context_->CreateObject<RigidBodyContactEntry>());
                    }
                    URHO3D_LOGINFO("PhysicsWorld Contact Entry Pool Grow To: " + String(physicsWorld_->contactEntryPool_.Size()));

                    physicsWorld_->contactEntryPoolCurIdx_ = prevSize;

                }

               
                
            }
            entry = physicsWorld_->contactEntryPool_[physicsWorld_->contactEntryPoolCurIdx_];


            //contactEntries_.InsertNew(otherBody->GetID(), newEntry);
            contactEntries_.Insert(Pair<unsigned int, RigidBodyContactEntry*>(otherBody->GetID(), entry));

        }

        return entry;
    }


    void RigidBody::CleanContactEntries()
    {
        Vector<unsigned int> keys = contactEntries_.Keys();
        for (int i = 0; i < keys.Size(); i++) {

            if (contactEntries_[keys[i]]->expired_)
                contactEntries_.Erase(keys[i]);
        }
    }

    void RigidBody::calculateSceneDepth()
    {
        sceneDepth_ = 0;
        Node* curNode = node_;
        while (curNode != GetScene())
        {
            curNode = curNode->GetParent();
            sceneDepth_++;
        }
    }

    void RigidBody::freeBody()
    {
        if (newtonBody_ != nullptr) {
            physicsWorld_->addToFreeQueue(newtonBody_);
            NewtonBodySetUserData(newtonBody_, nullptr);
            newtonBody_ = nullptr;
        }


        //also free the compound collision if there is one
        if (effectiveCollision_)
        {
            physicsWorld_->addToFreeQueue(effectiveCollision_);
            effectiveCollision_ = nullptr;
        }
        
        //mark all contact entries as expired so physicsworld pool is more free.
        Vector<unsigned int> keys = contactEntries_.Keys();
        for (int i = 0; i < keys.Size(); i++) {
            contactEntries_[keys[i]]->expired_ = true;
        }
    }


    //this function looks scary - But it covers alot of ground.
    void RigidBody::reBuildBody()
    {
        URHO3D_PROFILE_FUNCTION();

        freeBody();
        dMatrix finalInertia;
        dVector finalCenterOfMass;
        dMatrix identity = dGetIdentityMatrix();
        newtonBody_ = NewtonCreateDynamicBody(physicsWorld_->GetNewtonWorld(), nullptr, &identity[0][0]);

        for (int densityPass = 1; densityPass >= 0; densityPass--)
        {

            if (!IsEnabledEffective())
                return;


            //evaluate child nodes (+this node) and see if there are more collision shapes - if so create a compound collision.
            PODVector<CollisionShape*> childCollisionShapes;

            GetAloneCollisionShapes(childCollisionShapes, node_, true);


            PODVector<CollisionShape*> filteredList;

            //update member list of shapes.
            collisionShapes_ = childCollisionShapes;


            //filter out shapes that are not enabled.
            for (CollisionShape* col : childCollisionShapes)
            {
                if (col->IsEnabledEffective() && col->GetNewtonCollision())
                    filteredList += col;
            }
            childCollisionShapes = filteredList;


            if (childCollisionShapes.Size())
            {



                NewtonCollision* resolvedCollision = nullptr;

                if (effectiveCollision_) {
                    NewtonDestroyCollision(effectiveCollision_);
                    effectiveCollision_ = nullptr;
                }


                ///determine early on if a compound is going to be needed.
                bool compoundNeeded = false;
                float smallestDensity = M_LARGE_VALUE;
                for (CollisionShape* col : childCollisionShapes)
                {
                    if (col->IsCompound())
                        compoundNeeded = true;


                    if (col->GetDensity() < smallestDensity)
                        smallestDensity = col->GetDensity();
                }
                compoundNeeded |= (childCollisionShapes.Size() > 1);




                if (compoundNeeded) {
                    if (sceneRootBodyMode_)
                        effectiveCollision_ = NewtonCreateSceneCollision(physicsWorld_->GetNewtonWorld(), 0);//internally the same as a regular compond with some flags enabled..
                    else
                        effectiveCollision_ = NewtonCreateCompoundCollision(physicsWorld_->GetNewtonWorld(), 0);

                    NewtonCompoundCollisionBeginAddRemove(effectiveCollision_);
                }
                float accumMass = 0.0f;

                CollisionShape* firstCollisionShape = nullptr;
                for (CollisionShape* colComp : childCollisionShapes)
                {
                    if (firstCollisionShape == nullptr)
                        firstCollisionShape = colComp;

                    //for each sub collision in the colComp
                    const NewtonCollision* rootCollision = colComp->GetNewtonCollision();

                    void* curSubNode = NewtonCompoundCollisionGetFirstNode((NewtonCollision*)rootCollision);
                    NewtonCollision* curSubCollision = nullptr;
                    if (curSubNode)
                        curSubCollision = NewtonCompoundCollisionGetCollisionFromNode((NewtonCollision*)rootCollision, curSubNode);
                    else
                        curSubCollision = (NewtonCollision*)rootCollision;

                    while (curSubCollision)
                    {
                        NewtonCollision* curCollisionInstance = NewtonCollisionCreateInstance(curSubCollision);
                        curSubNode = NewtonCompoundCollisionGetNextNode((NewtonCollision*)rootCollision, curSubNode);//advance
                        if (curSubNode)
                            curSubCollision = NewtonCompoundCollisionGetCollisionFromNode((NewtonCollision*)rootCollision, curSubNode);
                        else
                            curSubCollision = nullptr;


                        Quaternion colPhysworldRot = colComp->GetWorldRotation();
                        Quaternion thisNodeWorldRot = node_->GetWorldRotation();
                        Quaternion colRotLocalToThisNode = thisNodeWorldRot.Inverse() * colPhysworldRot;

                        //compute the relative vector from root node to collision
                        Vector3 relativePos = (node_->GetRotation().Inverse()*(colComp->GetWorldPosition() - node_->GetWorldPosition()));

                        //form final local matrix with physics world scaling applied.
                        Matrix3x4 nodeWorldNoScale(node_->GetWorldTransform().Translation(), node_->GetWorldTransform().Rotation(), 1.0f);
                        Matrix3x4 colWorldNoScale(colComp->GetWorldTransform().Translation(), colComp->GetWorldTransform().Rotation(), 1.0f);


                        Matrix3x4 finalLocal = nodeWorldNoScale.Inverse() * colWorldNoScale;

                        dMatrix localTransform = UrhoToNewton(Matrix3x4(physicsWorld_->SceneToPhysics_Domain(finalLocal.Translation()), colRotLocalToThisNode, 1.0f));


                        //now determine scale to apply around the center of each sub shape.
                        Vector3 scale = Vector3::ONE;
                        if (colComp->GetInheritNodeScale())
                        {
                            scale = colComp->GetRotationOffset().Inverse() * colComp->GetNode()->GetWorldScale();
                        }
                        Vector3 shapeScale = colComp->GetScaleFactor();

                        scale = scale * shapeScale;
                        scale = physicsWorld_->SceneToPhysics_Domain(scale);

                        dVector existingLocalScale;
                        NewtonCollisionGetScale(curCollisionInstance, &existingLocalScale.m_x, &existingLocalScale.m_y, &existingLocalScale.m_z);


                        float densityScaleFactor = 1.0f;
                        //if we are in the first pass - scale the sub collision by the density.  so when we calculate the intertia matrix it will reflect the density of subshapes.
                        //on the 2nd (final pass - scale as normal).
                        if (densityPass)
                            densityScaleFactor = colComp->GetDensity()/smallestDensity;

                        NewtonCollisionSetScale(curCollisionInstance, densityScaleFactor*scale.x_*existingLocalScale.m_x, densityScaleFactor*scale.y_*existingLocalScale.m_y, densityScaleFactor*scale.z_*existingLocalScale.m_z);




                        //take into account existing local matrix of the newton collision shape.
                        dMatrix existingLocalMatrix;
                        NewtonCollisionGetMatrix(curCollisionInstance, &existingLocalMatrix[0][0]);

                        Vector3 subLocalPos = NewtonToUrhoVec3(existingLocalMatrix.m_posit);
                        subLocalPos = (subLocalPos * Vector3(scale.x_*existingLocalScale.m_x, scale.y_*existingLocalScale.m_y, scale.z_*existingLocalScale.m_z));
                        subLocalPos = colComp->GetRotationOffset() * subLocalPos;
                        existingLocalMatrix.m_posit = UrhoToNewton(subLocalPos);


                        localTransform = existingLocalMatrix * localTransform;
                        NewtonCollisionSetMatrix(curCollisionInstance, &localTransform[0][0]);//set the collision matrix with translation and rotation data only.


                        //calculate volume
                        float vol = NewtonConvexCollisionCalculateVolume(curCollisionInstance);
                        accumMass += vol * colComp->GetDensity();


                        //end adding current shape.
                        if (compoundNeeded) {

                            if (sceneRootBodyMode_)
                                NewtonSceneCollisionAddSubCollision(effectiveCollision_, curCollisionInstance);
                            else
                                NewtonCompoundCollisionAddSubCollision(effectiveCollision_, curCollisionInstance);

                            NewtonDestroyCollision(curCollisionInstance);//free the temp collision that was used to build the compound.
                        }
                        else
                            resolvedCollision = curCollisionInstance;


                    }
                }
                if (compoundNeeded) {

                    NewtonCompoundCollisionEndAddRemove(effectiveCollision_);

                    resolvedCollision = effectiveCollision_;
                }

                effectiveCollision_ = resolvedCollision;


                //create the body at node transform (with physics world scale applied)
                Matrix3x4 worldTransform;

                worldTransform.SetTranslation(physicsWorld_->SceneToPhysics_Domain(node_->GetWorldPosition()));
                worldTransform.SetRotation((node_->GetWorldRotation()).RotationMatrix());

                
                //NewtonBody* body = NewtonCreateDynamicBody(physicsWorld_->GetNewtonWorld(), resolvedCollision, &UrhoToNewton(worldTransform)[0][0]);

                NewtonBodySetCollision(newtonBody_, resolvedCollision);
                NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(worldTransform)[0][0]);


                targetNodeRotation_ = node_->GetWorldRotation();
                targetNodePos_ = node_->GetWorldPosition();
                SnapInterpolation();




                mass_ = accumMass * massScale_;
                if (sceneRootBodyMode_)
                    mass_ = 0;

                if (densityPass) {
                    NewtonBodySetMassProperties(newtonBody_, mass_, resolvedCollision);

                    //save the inertia matrix for 2nd pass.
                
                    NewtonBodyGetInertiaMatrix(newtonBody_, &finalInertia[0][0]);
                    
                    NewtonBodyGetCentreOfMass(newtonBody_, &finalCenterOfMass[0]);
                }


                

               

            }
        }


        Matrix4 inertiaMatrixUrho = NewtonToUrhoMat4(finalInertia);
       // URHO3D_LOGINFO("Inertia Matrix:");
        //URHO3D_LOGINFO(String(inertiaMatrixUrho));


        //test if the inertia matrix is symetric.
        //URHO3D_LOGINFO("Final Mass: " + String(mass_));



        NewtonBodySetFullMassMatrix(newtonBody_, mass_, &finalInertia[0][0]);
        NewtonBodySetCentreOfMass(newtonBody_, &finalCenterOfMass[0]);


        NewtonBodySetMaterialGroupID(newtonBody_, 0);

        NewtonBodySetUserData(newtonBody_, (void*)this);

        NewtonBodySetContinuousCollisionMode(newtonBody_, continuousCollision_);

        //ensure newton damping is 0 because we apply our own as a force.
        NewtonBodySetLinearDamping(newtonBody_, linearDampeningInternal_);
        NewtonBodySetAngularDamping(newtonBody_, &UrhoToNewton(angularDampeningInternal_)[0]);

        //set auto sleep mode.
        NewtonBodySetAutoSleep(newtonBody_, autoSleep_);


        //assign callbacks
        NewtonBodySetForceAndTorqueCallback(newtonBody_, Newton_ApplyForceAndTorqueCallback);
        NewtonBodySetTransformCallback(newtonBody_, Newton_SetTransformCallback);
        NewtonBodySetDestructorCallback(newtonBody_, Newton_DestroyBodyCallback);
    }






    void RigidBody::updateInterpolatedTransform()
    {
        
        interpolatedNodePos_ += (targetNodePos_ - interpolatedNodePos_)*interpolationFactor_;
        interpolatedNodeRotation_ = interpolatedNodeRotation_.Nlerp(targetNodeRotation_, interpolationFactor_, true);
    }


    bool RigidBody::InterpolationWithinRestTolerance()
    {
        bool inTolerance = true;
        inTolerance &= ( (targetNodePos_ - interpolatedNodePos_).Length() < M_EPSILON );
        inTolerance &= ( (targetNodeRotation_ - interpolatedNodeRotation_).Angle() < M_EPSILON);

        return inTolerance;
    }

    void RigidBody::SnapInterpolation()
    {
        interpolatedNodePos_ = targetNodePos_;
        interpolatedNodeRotation_ = targetNodeRotation_;
    }

    void RigidBody::OnNodeSet(Node* node)
    {
        if (node)
        {

            //Auto-create a physics world on the scene if it does not yet exist.
            physicsWorld_ = WeakPtr<PhysicsWorld>(GetScene()->GetOrCreateComponent<PhysicsWorld>());

            physicsWorld_->addRigidBody(this);

            node->AddListener(this);

            calculateSceneDepth();
            physicsWorld_->markRigidBodiesNeedSorted();

            //SetWorldTransform(node->GetWorldTransform());
            
            
            prevNode_ = node;
        }
        else
        {

            if (physicsWorld_)
                physicsWorld_->removeRigidBody(this);

            //#todo ?
            ////remove any connected constraints.
            //for (Constraint* constraint : connectedConstraints_) {
            //    constraint->Remove();
            //}



            freeBody();

            prevNode_ = nullptr;
        }

    }

    void RigidBody::OnSceneSet(Scene* scene)
    {

    }
    void RigidBody::HandleNodeAdded(StringHash event, VariantMap& eventData)
    {
        Node* node = static_cast<Node*>(eventData[NodeAdded::P_NODE].GetPtr());
        Node* newParent = static_cast<Node*>(eventData[NodeRemoved::P_PARENT].GetPtr());

        if (node == node_)
        {
            RebuildPhysicsNodeTree(node);
            calculateSceneDepth();
            physicsWorld_->markRigidBodiesNeedSorted();
        }
    }

    void RigidBody::HandleNodeRemoved(StringHash event, VariantMap& eventData)
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


   





    void RigidBody::applyDefferedActions()
    {
        //wake the body so it responds.
        Activate();


        if (nextPositionNeeded_ && !nextTransformNeeded_)
        {
            if (newtonBody_)
            {
                dgQuaternion orientation;
                NewtonBodyGetRotation(newtonBody_, &orientation.m_x);

                Matrix3x4 transform(nextPosition_, NewtonToUrhoQuat(orientation), 1.0f);
                NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(transform))[0][0]);
                
                nextPositionNeeded_ = false;
            }
        }

        if (nextOrientationNeeded_ && !nextTransformNeeded_)
        {
            if (newtonBody_)
            {
                dVector pos;
                NewtonBodyGetPosition(newtonBody_, &pos[0]);

                Matrix3x4 transform(NewtonToUrhoVec3(pos), nextOrientation_, 1.0f);
                NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(transform))[0][0]);
                
                nextOrientationNeeded_ = false;
            }
            
        }

        if (nextTransformNeeded_)
        {

            if (newtonBody_)
            {
                Matrix3x4 scaleLessTransform(nextTransform_.Translation(), nextTransform_.Rotation(), 1.0f);
                NewtonBodySetMatrix(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(scaleLessTransform))[0][0]);
                
                nextTransformNeeded_ = false;
            }
            
        }

        if (nextLinearVelocityNeeded_)
        {
            if (newtonBody_)
            {
                if (nextLinearVelocityUseForces_) {
                    dVector curWorldVel;
                    NewtonBodyGetVelocity(newtonBody_, &curWorldVel[0]);

                    dVector worldVel = UrhoToNewton(nextLinearVelocity_) - curWorldVel;
                    dVector bodyWorldPos;
                    NewtonBodyGetPosition(newtonBody_, &bodyWorldPos[0]);
                    NewtonBodyAddImpulse(newtonBody_, &worldVel[0], &bodyWorldPos[0], physicsWorld_->timeStepTarget_);
                }
                else
                    NewtonBodySetVelocity(newtonBody_, &UrhoToNewton(nextLinearVelocity_)[0]);
                
                nextLinearVelocityNeeded_ = false;
            }
            
        }
        if (nextAngularVelocityNeeded_)
        {
            if (newtonBody_)
            {
                NewtonBodySetOmega(newtonBody_, &UrhoToNewton(nextAngularVelocity_)[0]);
                nextAngularVelocityNeeded_ = false;
            }
            
        }
        if (nextImpulseNeeded_)
        {
            if (newtonBody_) {
                NewtonBodyAddImpulse(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(nextImpulseWorldVelocity_))[0],
                    &UrhoToNewton(node_->LocalToWorld(nextImpulseLocalPos_))[0], physicsWorld_->timeStepTarget_);
                
                nextImpulseNeeded_ = false;
            }

        }


        if (nextSleepStateNeeded_) {

            if (newtonBody_)
            {
                NewtonBodySetSleepState(newtonBody_, nextSleepState_);
                nextSleepStateNeeded_ = false;
            }
        }


    }

    void RigidBody::OnNodeSetEnabled(Node* node)
    {
        if (node == node_)
        {
            if (IsEnabledEffective()) {
                MarkDirty(true);//rebuild
            }
            else
            {
                freeBody();
            }
        }
    }

    void RigidBody::AddWorldForce(const Vector3& force)
    {
        AddWorldForce(force, Vector3::ZERO);
    }

    void RigidBody::AddWorldForce(const Vector3& worldForce, const Vector3& worldPosition)
    {
        netForce_ += worldForce ; 
        AddWorldTorque((worldPosition - GetCenterOfMassPosition()).CrossProduct(worldForce));
    }

    void RigidBody::AddWorldTorque(const Vector3& torque)
    {
        netTorque_ += torque;
    }

    void RigidBody::AddLocalForce(const Vector3& force)
    {
        AddWorldForce(node_->GetWorldRotation() * force);
    }

    void RigidBody::AddLocalForce(const Vector3& localForce, const Vector3& localPosition)
    {
        AddWorldForce(node_->GetWorldRotation() * localForce, GetCenterOfMassTransform() * (localPosition));
    }

    void RigidBody::AddLocalTorque(const Vector3& torque)
    {
        AddWorldTorque(node_->GetWorldRotation() * torque);
    }

    void RigidBody::ResetForces()
    {
        netForce_ = Vector3(0, 0, 0);
        netTorque_ = Vector3(0, 0, 0);
    }

    void RigidBody::AddImpulse(const Vector3& localPosition, const Vector3& targetVelocity)
    {


        if (newtonBody_) {
            Activate();
            NewtonBodyAddImpulse(newtonBody_, &UrhoToNewton(physicsWorld_->SceneToPhysics_Domain(targetVelocity))[0],
                &UrhoToNewton(node_->LocalToWorld(localPosition))[0], physicsWorld_->timeStepTarget_);
        }
        else
        {
            //schedule the impulse
            nextImpulseNeeded_ = true;
            nextImpulseLocalPos_ = localPosition;
            nextImpulseWorldVelocity_ = targetVelocity;
        }
        
    }

    Vector3 RigidBody::GetNetForce()
    {
        return physicsWorld_->PhysicsToScene_Domain(netForce_);
    }


    Urho3D::Vector3 RigidBody::GetNetTorque()
    {
        return physicsWorld_->PhysicsToScene_Domain(netTorque_);
    }

    NewtonCollision* RigidBody::GetEffectiveNewtonCollision() const
    {
        if (effectiveCollision_)
            return effectiveCollision_;

        return nullptr;
    }

    Urho3D::Vector3 RigidBody::GetLinearVelocity(TransformSpace space /*= TS_WORLD*/) const
{
        if (newtonBody_) {

            dVector dVel;
            NewtonBodyGetVelocity(newtonBody_, &dVel[0]);
            Vector3 vel = physicsWorld_->PhysicsToScene_Domain(NewtonToUrhoVec3(dVel));

            if (space == TS_WORLD)
            {
                return vel;
            }
            else if (space == TS_LOCAL)
            {
                return node_->WorldToLocal(vel);
            }
            else if (space == TS_PARENT)
            {
                return node_->GetParent()->WorldToLocal(vel);
            }

            return vel;
        }
        else
            return Vector3::ZERO;
    }

    Urho3D::Vector3 RigidBody::GetAngularVelocity(TransformSpace space /*= TS_WORLD*/) const
    {
        if (newtonBody_) {
            dVector dAngularVel;
            NewtonBodyGetOmega(newtonBody_, &dAngularVel[0]);
            Vector3 angularVel = physicsWorld_->PhysicsToScene_Domain(NewtonToUrhoVec3(dAngularVel));
            if (space == TS_WORLD)
            {
                return angularVel;
            }
            else if(space == TS_LOCAL)
            {
                return node_->WorldToLocal(angularVel);
            }
            else if (space == TS_PARENT)
            {
                return node_->GetParent()->WorldToLocal(angularVel);
            }
            return angularVel;
        }
        else
            return Vector3::ZERO;
    }


    Vector3 RigidBody::GetAcceleration()
    {
        if (newtonBody_) {
            dVector dAcc;
            NewtonBodyGetAcceleration(newtonBody_, &dAcc[0]);
            Vector3 acc = physicsWorld_->PhysicsToScene_Domain(NewtonToUrhoVec3(dAcc));
            return acc;
        }
        else
            return Vector3::ZERO;
    }

    void RigidBody::GetConnectedContraints(PODVector<Constraint*>& contraints)
    {
        contraints.Clear();
        for (auto i = connectedConstraints_.Begin(); i != connectedConstraints_.End(); ++i)
        {
            contraints += *i;
        }
    }

    Urho3D::PODVector<Constraint*> RigidBody::GetConnectedContraints()
    {
        PODVector<Constraint*> contraints;
        GetConnectedContraints(contraints);
        return contraints;
    }



    void RigidBody::ApplyTransform(float timestep)
{
        if (!newtonBody_)
            return;

        dVector pos;
        dQuaternion quat;
        NewtonBodyGetPosition(newtonBody_, &pos[0]);
        NewtonBodyGetRotation(newtonBody_, &quat.m_x);




        targetNodePos_ = physicsWorld_->PhysicsToScene_Domain(NewtonToUrhoVec3(pos));
        targetNodeRotation_ = NewtonToUrhoQuat(quat);


        updateInterpolatedTransform();

        node_->SetWorldTransform(interpolatedNodePos_, interpolatedNodeRotation_);
    }


    void RigidBody::GetForceAndTorque(Vector3& force, Vector3& torque)
    {
        URHO3D_PROFILE("GetForceAndTorque");

        //basic velocity damping forces
        Vector3 velocity = GetLinearVelocity(TS_WORLD);
        Vector3 linearDampingForce = -velocity.Normalized()*(velocity.LengthSquared())*linearDampening_ * mass_;

        if (linearDampingForce.Length() <= M_EPSILON)
            linearDampingForce = Vector3::ZERO;


        //basic angular damping forces
        Vector3 angularVelocity = GetAngularVelocity(TS_WORLD);
        Vector3 angularDampingTorque = -angularVelocity.Normalized()*(angularVelocity.LengthSquared())*angularDampening_ * mass_;

        if (angularVelocity.Length() <= M_EPSILON)
            angularDampingTorque = Vector3::ZERO;


        force = linearDampingForce + netForce_;
        torque = angularDampingTorque + netTorque_;

        
    }

}
