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
#include "../Core/Mutex.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "PhysicsWorld.h"
#include "IO/Log.h"
#include "CollisionShape.h"
#include "CollisionShapesDerived.h"
#include "RigidBody.h"
#include "dMatrix.h"
#include "Core/Context.h"
#include "Core/CoreEvents.h"
#include "Core/Object.h"
#include "UrhoNewtonConversions.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"
#include "Scene/Node.h"
#include "../Graphics/Model.h"
#include "Math/Sphere.h"
#include "Container/Vector.h"

#include "Newton.h"
#include "NewtonMeshObject.h"
#include "Core/Thread.h"
#include "Constraint.h"
#include "FixedDistanceConstraint.h"
#include "Core/Profiler.h"
#include "PhysicsEvents.h"
#include "BallAndSocketConstraint.h"
#include "NewtonKinematicsJoint.h"
#include "NewtonDebugDrawing.h"
#include "dgMatrix.h"
#include "dCustomJoint.h"
#include "Graphics/DebugRenderer.h"
#include "FullyFixedConstraint.h"
#include "HingeConstraint.h"
#include "Scene/SceneEvents.h"
#include "SliderConstraint.h"
#include "6DOFConstraint.h"
#include "Engine/Engine.h"
#include "Math/Ray.h"

namespace Urho3D {


    static const Vector3 DEFAULT_GRAVITY = Vector3(0.0f, -9.81f, 0.0f);

    PhysicsWorld::PhysicsWorld(Context* context) : Component(context)
    {
        SubscribeToEvent(E_SCENESUBSYSTEMUPDATE, URHO3D_HANDLER(PhysicsWorld, HandleSceneUpdate));


        contactEntryPool_.Clear();
        for (int i = 0; i < contactEntryPoolSize_; i++)
        {
            contactEntryPool_.Insert(0, context->CreateObject<RigidBodyContactEntry>());
        }

        //set timestep target to max fps
        timeStepTarget_ = 1.0f / GetSubsystem<Engine>()->GetMaxFps();
    }

    PhysicsWorld::~PhysicsWorld()
    {
    }

    void PhysicsWorld::RegisterObject(Context* context)
    {
        context->RegisterFactory<PhysicsWorld>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(Component);
        URHO3D_ACCESSOR_ATTRIBUTE("Gravity", GetGravity, SetGravity, Vector3, DEFAULT_GRAVITY, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Physics Scale", GetPhysicsScale, SetPhysicsScale, float, 1.0f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Time Scale", GetTimeScale, SetTimeScale, float, 1.0f, AM_DEFAULT);
    }




    void PhysicsWorld::SerializeNewtonWorld(String fileName)
    {
        NewtonSerializeToFile(newtonWorld_, fileName.CString(), nullptr, nullptr);
    }



    
    String PhysicsWorld::GetSolverPluginName()
    {
        void* plugin = NewtonCurrentPlugin(newtonWorld_);
        return String(NewtonGetPluginString(newtonWorld_, plugin));
    }



    
    void PhysicsWorld::RayCast(PODVector<PhysicsRayCastIntersection>& intersections, const Ray& ray, float maxDistance, unsigned maxIntersections, unsigned collisionMask /*= M_MAX_UNSIGNED*/)
    {
        RayCast( intersections, ray.origin_, ray.origin_ + ray.direction_*maxDistance, maxIntersections, collisionMask);
    }

    void PhysicsWorld::RayCast(PODVector<PhysicsRayCastIntersection>& intersections, const Vector3& pointOrigin, const Vector3& pointDestination, unsigned maxIntersections /*= M_MAX_UNSIGNED*/, unsigned collisionMask /*= M_MAX_UNSIGNED*/)
    {
        PhysicsRayCastUserData data;

        Vector3 origin = SceneToPhysics_Domain(pointOrigin);
        Vector3 destination = SceneToPhysics_Domain(pointDestination);

        NewtonWorldRayCast(newtonWorld_, &UrhoToNewton(origin)[0], &UrhoToNewton(destination)[0], Newton_WorldRayCastFilterCallback, &data, NULL, 0);

        //sort the intersections by distance. we do this because the order that you get is based off bounding box intersection and that is not nessecarily the same of surface intersection order.
        if (data.intersections.Size() > 1)
            Sort(data.intersections.Begin(), data.intersections.End(), PhysicsRayCastIntersectionCompare);


        int intersectCount = 0;
        for (PhysicsRayCastIntersection& intersection : data.intersections)
        {

            unsigned collisionLayerAsBit = CollisionLayerAsBit(intersection.rigBody->GetCollisionLayer());


            if ((intersectCount <= maxIntersections)
                && (collisionLayerAsBit & collisionMask)) {


                intersection.rayIntersectWorldPosition = PhysicsToScene_Domain(intersection.rayIntersectWorldPosition);
                intersection.rayOriginWorld = pointOrigin;
                intersection.rayDistance = (intersection.rayIntersectWorldPosition - pointOrigin).Length();

                intersections += intersection;
                intersectCount++;

            }
        }

    }


    void PhysicsWorld::SetGravity(const Vector3& force)
    {
        gravity_ = force;
    }


    Urho3D::Vector3 PhysicsWorld::GetGravity() const
{
        return gravity_;
    }


    void PhysicsWorld::SetIterationCount(int numIterations /*= 8*/)
    {
        iterationCount_ = numIterations;
        applyNewtonWorldSettings();
    }


    int PhysicsWorld::GetIterationCount() const
    {
        return iterationCount_;
    }

    void PhysicsWorld::SetSubstepFactor(int factor)
    {
        subSteps_ = factor;
        applyNewtonWorldSettings();
    }

    int PhysicsWorld::GetSubstepFactor() const
    {
        return subSteps_;
    }

    void PhysicsWorld::SetThreadCount(int numThreads)
    {
        newtonThreadCount_ = numThreads;
        applyNewtonWorldSettings();
    }

    int PhysicsWorld::GetThreadCount() const
    {
        return newtonThreadCount_;
    }

    void PhysicsWorld::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        URHO3D_PROFILE_FUNCTION();
        if (debug)
        {
            //draw debug geometry on constraints.
            for (Constraint* constraint : constraintList)
            {
                constraint->DrawDebugGeometry(debug, depthTest);
            }

            //draw debug geometry for contacts
            for (int i = 0; i < contactEntryPool_.Size(); i++)
            {
               if(!contactEntryPool_[i]->expired_)
                   contactEntryPool_[i]->DrawDebugGeometry(debug, depthTest);
            }


            //draw debug geometry on rigid bodies.
            for (RigidBody* body : rigidBodyComponentList) {
                body->DrawDebugGeometry(debug, depthTest);
            }

            //draw debug geometry on static scene.
            if (sceneBody_)
                sceneBody_->DrawDebugGeometry(debug, depthTest);


        }
    }

    void PhysicsWorld::OnSceneSet(Scene* scene)
    {
        if (scene) {
            //create the newton world
            if (newtonWorld_ == nullptr) {
                newtonWorld_ = NewtonCreate();
                NewtonWorldSetUserData(newtonWorld_, (void*)this);
                applyNewtonWorldSettings();


                NewtonMaterialSetCollisionCallback(newtonWorld_, 0, 0, Newton_AABBOverlapCallback, Newton_ProcessContactsCallback);
                //NewtonMaterialSetCompoundCollisionCallback(newtonWorld_, 0, 0, Newton_AABBCompoundOverlapCallback);





            }
        }
        else
        {
            //wait for update to finish if in async mode so we can safely clean up.
            NewtonWaitForUpdateToFinish(newtonWorld_);

            freeWorld();
        }
    }

    void PhysicsWorld::addCollisionShape(CollisionShape* collision)
    {
        collisionComponentList.Insert(0, WeakPtr<CollisionShape>(collision));
    }

    void PhysicsWorld::removeCollisionShape(CollisionShape* collision)
    {
        collisionComponentList.Remove(WeakPtr<CollisionShape>(collision));
    }

    void PhysicsWorld::addRigidBody(RigidBody* body)
    {
        rigidBodyComponentList.Insert(0, WeakPtr<RigidBody>(body));
    }

    void PhysicsWorld::removeRigidBody(RigidBody* body)
    {
        rigidBodyComponentList.Remove(WeakPtr<RigidBody>(body));
    }

    void PhysicsWorld::addConstraint(Constraint* constraint)
    {
        constraintList.Insert(0, WeakPtr<Constraint>(constraint));
    }

    void PhysicsWorld::removeConstraint(Constraint* constraint)
    {
        constraintList.Remove(WeakPtr<Constraint>(constraint));
    }



    void PhysicsWorld::freeWorld()
    {

        //free any joints
        for (Constraint* constraint : constraintList)
        {
            constraint->freeInternal();
        }
        constraintList.Clear();

        //free any collision shapes currently in the list
        for (CollisionShape* col : collisionComponentList)
        {
            col->freeInternalCollision();
        }
        collisionComponentList.Clear();



        //free internal bodies for all rigid bodies.
        for (RigidBody* rgBody : rigidBodyComponentList)
        {
            rgBody->freeBody();
        }
        rigidBodyComponentList.Clear();



        //free meshes in mesh cache
        newtonMeshCache_.Clear();

        //free the actual memory
        freePhysicsInternals();



        //destroy newton world.
        if (newtonWorld_ != nullptr) {
            NewtonDestroy(newtonWorld_);
            newtonWorld_ = nullptr;
        }


    }




    void PhysicsWorld::addToFreeQueue(NewtonBody* newtonBody)
    {
        freeBodyQueue_ += newtonBody;
    }

    void PhysicsWorld::addToFreeQueue(dCustomJoint* newtonConstraint)
    {
        freeConstraintQueue_ += newtonConstraint;
    }

    void PhysicsWorld::addToFreeQueue(NewtonCollision* newtonCollision)
    {
        freeCollisionQueue_ += newtonCollision;
    }

    void PhysicsWorld::applyNewtonWorldSettings()
    {
        NewtonSetSolverIterations(newtonWorld_, iterationCount_);
        NewtonSetNumberOfSubsteps(newtonWorld_, 1);
        NewtonSetThreadsCount(newtonWorld_, newtonThreadCount_);
        NewtonSelectBroadphaseAlgorithm(newtonWorld_, 1);//persistent broadphase.

        
    }

    void PhysicsWorld::formContacts(bool rootrate)
{
        URHO3D_PROFILE_FUNCTION();

        for (RigidBody* rigBody : rigidBodyComponentList)
        {
            if (!rigBody->generateContacts_)
                continue;

            NewtonBody* newtonBody = rigBody->GetNewtonBody();
            if(!newtonBody)
                continue;


            NewtonJoint* curJoint = NewtonBodyGetFirstContactJoint(newtonBody);
            while (curJoint) {


                NewtonBody* body0 = NewtonJointGetBody0(curJoint);
                NewtonBody* body1 = NewtonJointGetBody1(curJoint);

                RigidBody* rigBody0 = (RigidBody*)NewtonBodyGetUserData(body0);
                RigidBody* rigBody1 = (RigidBody*)NewtonBodyGetUserData(body1);

                if (!rigBody0 || !rigBody1)
                {
                    curJoint = NewtonBodyGetNextContactJoint(newtonBody, curJoint);
                    continue;
                }

                if (!rigBody0->generateContacts_ || !rigBody1->generateContacts_) {
                    curJoint = NewtonBodyGetNextContactJoint(newtonBody, curJoint);
                    continue;
                }



                SharedPtr<RigidBodyContactEntry> contactEntry = nullptr;
                contactEntry = rigBody0->GetCreateContactEntry(rigBody1);
                


                if (contactEntry->expired_) {
                    contactEntry->body0 = rigBody0;
                    contactEntry->body1 = rigBody1;

                    contactEntry->expired_ = false;
                    contactEntry->numContacts = 0;
                }

                if(NewtonJointIsActive(curJoint))
                    contactEntry->wakeFlag_ = NewtonJointIsActive(curJoint);

                if(NewtonContactJointGetContactCount(curJoint) > contactEntry->numContacts)
                    contactEntry->numContacts = NewtonContactJointGetContactCount(curJoint);




                //rigBody0->CleanContactEntries();




                if (contactEntry->numContacts > DEF_PHYSICS_MAX_CONTACT_POINTS)
                {
                    URHO3D_LOGWARNING("Contact Entry Contact Count Greater Than DEF_PHYSICS_MAX_CONTACT_POINTS, consider increasing the limit.");
                }


                int contactIdx = 0;
                for (void* contact = NewtonContactJointGetFirstContact(curJoint); contact; contact = NewtonContactJointGetNextContact(curJoint, contact))
                {
                    
                    NewtonMaterial* const material = NewtonContactGetMaterial(contact);

                    NewtonCollision* shape0 = NewtonMaterialGetBodyCollidingShape(material, body0);
                    NewtonCollision* shape1 = NewtonMaterialGetBodyCollidingShape(material, body1);


                    CollisionShape* colShape0 = static_cast<CollisionShape*>(NewtonCollisionGetUserData(shape0));
                    CollisionShape* colShape1 = static_cast<CollisionShape*>(NewtonCollisionGetUserData(shape1));




                    //get contact geometric info for the contact struct
                    dVector pos, force, norm, tan0, tan1;
                    NewtonMaterialGetContactPositionAndNormal(material, body0, &pos[0], &norm[0]);
                    NewtonMaterialGetContactTangentDirections(material, body0, &tan0[0], &tan1[0]);
                    NewtonMaterialGetContactForce(material, body0, &force[0]);


                    contactEntry->contactNormals[contactIdx] = PhysicsToScene_Domain(NewtonToUrhoVec3(norm));
                    contactEntry->contactPositions[contactIdx] = PhysicsToScene_Domain(NewtonToUrhoVec3(pos));
                    contactEntry->contactTangent0[contactIdx] = PhysicsToScene_Domain(NewtonToUrhoVec3(tan0));
                    contactEntry->contactTangent1[contactIdx] = PhysicsToScene_Domain(NewtonToUrhoVec3(tan1));
                    contactEntry->contactForces[contactIdx] = PhysicsToScene_Domain(NewtonToUrhoVec3(force));


                    contactEntry->shapes0[contactIdx] = colShape0;
                    contactEntry->shapes1[contactIdx] = colShape1;

                    //#todo debugging
                    //GSS<VisualDebugger>()->AddCross(contactEntry->contactPositions[contactIdx], 0.1f, Color::BLUE, true);

                    contactIdx++;
                }
                
                curJoint = NewtonBodyGetNextContactJoint(newtonBody, curJoint);
            }

        }









    }

    void PhysicsWorld::ParseContacts()
    {
        PODVector<unsigned int> removeKeys;
        VariantMap eventData;
        eventData[PhysicsCollisionStart::P_WORLD] = this;


        for (int i = 0; i < rigidBodyComponentList.Size(); i++)
        {
            RigidBody* rigBody = rigidBodyComponentList[i];
            if(!rigBody->newtonBody_ || !rigBody->effectiveCollision_)
                continue;

            if (!rigBody->generateContacts_)
                continue;

            for (HashMap<unsigned int, RigidBodyContactEntry*>::ConstIterator i = rigBody->contactEntries_.Begin(); i != rigBody->contactEntries_.End(); i++)
            {
                RigidBodyContactEntry* entry = i->second_;

                if (entry->expired_)
                    continue;

                eventData[PhysicsCollisionStart::P_BODYA] = entry->body0;
                eventData[PhysicsCollisionStart::P_BODYB] = entry->body1;

                eventData[PhysicsCollisionStart::P_CONTACT_DATA] = entry;

                if (!entry->body0.Refs() || !entry->body1.Refs())//check expired
                {
                    entry->expired_ = true;
                }
                else if (entry->wakeFlag_ && !entry->wakeFlagPrev_)//begin contact
                {
                    if (entry->body0->collisionEventMode_ &&entry->body1->collisionEventMode_) {
                        SendEvent(E_PHYSICSCOLLISIONSTART, eventData);
                    }

                    if (entry->body0->collisionEventMode_) {
                        if (!entry->body0.Refs() || !entry->body1.Refs()) break; //it is possible someone deleted a body in the previous event.

                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body1->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body1;
                        entry->body0->GetNode()->SendEvent(E_NODECOLLISIONSTART, eventData);
                    }


                    if (entry->body1->collisionEventMode_) {
                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;

                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body0->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body0;
                        entry->body1->GetNode()->SendEvent(E_NODECOLLISIONSTART, eventData);
                    }


                    if (entry->body0->collisionEventMode_ &&entry->body1->collisionEventMode_) {
                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;

                        //also send the E_NODECOLLISION event
                        SendEvent(E_PHYSICSCOLLISION, eventData);
                    }

                    if (entry->body0->collisionEventMode_) {

                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;

                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body1->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body1;
                        entry->body0->GetNode()->SendEvent(E_NODECOLLISION, eventData);
                    }


                    if (entry->body1->collisionEventMode_) {
                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;

                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body0->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body0;
                        entry->body1->GetNode()->SendEvent(E_NODECOLLISION, eventData);
                    }


                }
                else if (!entry->wakeFlag_ && entry->wakeFlagPrev_)//end contact
                {
                    if (entry->body0->collisionEventMode_ &&entry->body1->collisionEventMode_) {
                        SendEvent(E_PHYSICSCOLLISIONEND, eventData);
                    }


                    if (entry->body0->collisionEventMode_) {

                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;
                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body1->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body1;
                        entry->body0->GetNode()->SendEvent(E_NODECOLLISIONEND, eventData);
                    }

                    if (entry->body1->collisionEventMode_) {
                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;
                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body0->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body0;
                        entry->body1->GetNode()->SendEvent(E_NODECOLLISIONEND, eventData);
                    }
                }
                else if (entry->wakeFlag_ && entry->wakeFlagPrev_)//continued contact
                {
                    if (entry->body0->collisionEventMode_ &&entry->body1->collisionEventMode_) {
                        SendEvent(E_PHYSICSCOLLISION, eventData);
                    }



                    if (entry->body0->collisionEventMode_) {
                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;

                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body1->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body1;
                        entry->body0->GetNode()->SendEvent(E_NODECOLLISION, eventData);
                    }

                    if (entry->body1->collisionEventMode_) {

                        if (!entry->body0.Refs() || !entry->body1.Refs()) break;

                        eventData[NodeCollisionStart::P_OTHERNODE] = entry->body0->GetNode();
                        eventData[NodeCollisionStart::P_OTHERBODY] = entry->body0;
                        entry->body1->GetNode()->SendEvent(E_NODECOLLISION, eventData);
                    }
                }
                else if (!entry->wakeFlag_ && !entry->wakeFlagPrev_)//no contact for one update. (mark for removal from the map)
                {
                    entry->expired_ = true;
                }

                //move on..
                entry->wakeFlagPrev_ = entry->wakeFlag_;
                entry->wakeFlag_ = false;
            }

            if(rigBody->Refs() && rigBody->contactEntries_.Size() > 10)
                rigBody->CleanContactEntries();
        }


    }




    void PhysicsWorld::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
    {
       sceneUpdated_ = true;

       float timeStep = eventData[SceneSubsystemUpdate::P_TIMESTEP].GetFloat();
       timeStepTarget_ = timeStep;

       if (timeStep <= 0.0001f) {
           return;
       }

       //move the timestep target slowely towards the time step we actually get.  We dont want any sudden changes given to newton. (in fact ideally it is constant)
       if(timeStepTarget_ < timeStep)
           timeStepTarget_ += (timeStep - timeStepTarget_) * (1.0f / 32.0f);
       else if(timeStepTarget_ > timeStep)
           timeStepTarget_ += (timeStep - timeStepTarget_) * (1.0f / 32.0f);



       // Send pre-step event
       eventData = GetEventDataMap();
       eventData[PhysicsPreStep::P_WORLD] = this;
       eventData[PhysicsPreStep::P_TIMESTEP] = timeStep;
       SendEvent(E_PHYSICSPRESTEP, eventData);

       //do the update.
       Update(timeStep / float(subSteps_), true);
       for(int i = 0; i < subSteps_-1; i++)
            Update(timeStep / float(subSteps_), false);


       // Send post-step event
       eventData = GetEventDataMap();
       eventData[PhysicsPostStep::P_WORLD] = this;
       eventData[PhysicsPostStep::P_TIMESTEP] = timeStep;
       SendEvent(E_PHYSICSPOSTSTEP, eventData);
    }



    void PhysicsWorld::Update(float timestep, bool isRootUpdate)
    {

        URHO3D_PROFILE_FUNCTION();
        float timeStep = timestep*GetScene()->GetTimeScale()*timeScale_;
        bool rootRate = isRootUpdate;

        if (rootRate) {

            if (!sceneBody_) {
                sceneBody_ = GetScene()->GetOrCreateComponent<RigidBody>();
                sceneBody_->SetIsSceneRootBody(true);
            }
        }



        if (simulationStarted_) {
            URHO3D_PROFILE("Wait For ASync Update To finish.");
            
            NewtonWaitForUpdateToFinish(newtonWorld_);
        }
        
        if (rootRate) {

            if (simulationStarted_) {

                {
                    URHO3D_PROFILE("Apply Node Transforms");

                    {
                        URHO3D_PROFILE("Rigid Body Order Pre Sort");
                        //sort the rigidBodyComponentList by scene depth.
                        if (rigidBodyListNeedsSorted) {
                            Sort(rigidBodyComponentList.Begin(), rigidBodyComponentList.End(), RigidBodySceneDepthCompare);
                            rigidBodyListNeedsSorted = false;
                        }
                    }

                    //apply the transform of all rigid body components to their respective nodes.
                    for (RigidBody* rigBody : rigidBodyComponentList)
                    {
                        if (rigBody->GetInternalTransformDirty()) {
                            rigBody->ApplyTransform(timeStep);


                            if (rigBody->InterpolationWithinRestTolerance())
                                rigBody->MarkInternalTransformDirty(false);
                        }
                    }

                }
            }






        }

        formContacts(rootRate);
        
        if (rootRate) {

            freePhysicsInternals();

            //rebuild stuff.
            rebuildDirtyPhysicsComponents();

            ParseContacts();

        }

        {
            URHO3D_PROFILE("NewtonUpdate");
            //use target time step to give newton constant time steps. 


            NewtonUpdateAsync(newtonWorld_, timeStep);
            simulationStarted_ = true;
        }
    }

    void PhysicsWorld::rebuildDirtyPhysicsComponents()
    {
        URHO3D_PROFILE_FUNCTION();



        //rebuild dirty collision shapes
        for (CollisionShape* colShape : collisionComponentList)
        {
            if (colShape->GetDirty()) {
                colShape->updateBuild();
                colShape->MarkDirty(false);
            }
        }
        

        //then rebuild rigid bodies if they need rebuilt (dirty) from root nodes up.
        for (RigidBody* rigBody : rigidBodyComponentList)
        {
            if (!rigBody->GetDirty())
                continue;



            rigBody->reBuildBody();
            rigBody->MarkDirty(false);
        }


        //rebuild contraints if they need rebuilt (dirty)
        for (Constraint* constraint : constraintList)
        {
            if (constraint->needsRebuilt_)
                constraint->reEvalConstraint();
        }

        //apply deferred actions (like impulses/velocity sets etc.) that were waiting for a real body to be built.
        for (RigidBody* rigBody : rigidBodyComponentList)
        {
            rigBody->applyDefferedActions();
        }


    }





    Urho3D::StringHash PhysicsWorld::NewtonMeshKey(String modelResourceName, int modelLodLevel, String otherData)
    {
        return modelResourceName + String(modelLodLevel) + otherData;
    }

    NewtonMeshObject* PhysicsWorld::GetCreateNewtonMesh(StringHash urhoNewtonMeshKey)
    {
        if (newtonMeshCache_.Contains(urhoNewtonMeshKey)) {
            return newtonMeshCache_[urhoNewtonMeshKey];
        }
        else
        {
            NewtonMesh* mesh = NewtonMeshCreate(newtonWorld_);
            SharedPtr<NewtonMeshObject> meshObj = context_->CreateObject<NewtonMeshObject>();
            meshObj->mesh = mesh;
            newtonMeshCache_[urhoNewtonMeshKey] = meshObj;
            return meshObj;
        }
    }

    NewtonMeshObject* PhysicsWorld::GetNewtonMesh(StringHash urhoNewtonMeshKey)
    {
        if (newtonMeshCache_.Contains(urhoNewtonMeshKey)) {
            return newtonMeshCache_[urhoNewtonMeshKey];
        }
        return nullptr;
    }

    void PhysicsWorld::freePhysicsInternals()
    {
        for (dCustomJoint* constraint : freeConstraintQueue_)
        {
            delete constraint;
        }
        freeConstraintQueue_.Clear();


        for (NewtonCollision* col : freeCollisionQueue_)
        {
            NewtonDestroyCollision(col);
        }
        freeCollisionQueue_.Clear();


        for (NewtonBody* body : freeBodyQueue_)
        {
            NewtonDestroyBody(body);
        }
        freeBodyQueue_.Clear();




    }
 

    String NewtonThreadProfilerString(int threadIndex)
    {
        return (String("Newton_Thread") + String(threadIndex));
    }

    

    //add rigid bodies to the list as the function recurses from node to root. the last rigidbody in rigidBodies is the most root. optionally include the scene as root.
    void GetRootRigidBodies(PODVector<RigidBody*>& rigidBodies, Node* node, bool includeScene)
    {
        RigidBody* body = node->GetComponent<RigidBody>();


        if (body)
            rigidBodies += body;

        //recurse on parent
        if(node->GetParent() && ((node->GetScene() != node->GetParent()) || includeScene))
            GetRootRigidBodies(rigidBodies, node->GetParent(), includeScene);
    }


    URHO3D_API RigidBody*  GetRigidBody(Node* node, bool includeScene)
    {
        Node* curNode = node;

        while (curNode) {
            if (curNode == curNode->GetScene() && !includeScene)
            {
                return nullptr;
            }

            RigidBody* body = curNode->GetComponent<RigidBody>();

            if (body)
                return body;

            curNode = curNode->GetParent();
        }

        return nullptr;
    }

    //returns first occuring child rigid bodies.
    void URHO3D_API GetNextChildRigidBodies(PODVector<RigidBody*>& rigidBodies, Node* node)
    {

        PODVector<Node*> immediateChildren;
        node->GetChildren(immediateChildren, false);

        for (Node* child : immediateChildren) {
            if (child->HasComponent<RigidBody>())
                rigidBodies += child->GetComponent<RigidBody>();
            else
                GetNextChildRigidBodies(rigidBodies, child);
        }

    }

    //recurses up the scene tree starting a node and continuing up every branch adding collision shapes to the array until a rigid body is encountered in which case the algorithm stops traversing that branch.
    void GetAloneCollisionShapes(PODVector<CollisionShape*>& colShapes, Node* startingNode, bool includeStartingNodeShapes)
    {

        if (includeStartingNodeShapes)
        {
            startingNode->GetDerivedComponents<CollisionShape>(colShapes, false, false);
        }



        PODVector<Node*> immediateChildren;
        startingNode->GetChildren(immediateChildren, false);

        for (Node* child : immediateChildren) {
            if (child->HasComponent<RigidBody>())
                continue;
            else
            {
                child->GetDerivedComponents<CollisionShape>(colShapes, false, false);
                GetAloneCollisionShapes(colShapes, child, false);

            }

        }
    }









    void RebuildPhysicsNodeTree(Node* node)
    {
        //trigger a rebuild on the root of the new tree.
        PODVector<RigidBody*> rigBodies;
        GetRootRigidBodies(rigBodies, node, false);
        if (rigBodies.Size()) {
            RigidBody* mostRootRigBody = rigBodies.Back();
            if (mostRootRigBody)
                mostRootRigBody->MarkDirty(true);
        }
    }


    unsigned CollisionLayerAsBit(unsigned layer)
    {
        if (layer == 0)
            return M_MAX_UNSIGNED;

        return (0x1 << layer - 1);
    }

    void RegisterPhysicsLibrary(Context* context)
    {
        PhysicsWorld::RegisterObject(context);

        CollisionShape::RegisterObject(context);
        CollisionShape_Box::RegisterObject(context);
        CollisionShape_Sphere::RegisterObject(context);
        CollisionShape_Cylinder::RegisterObject(context);
        CollisionShape_ChamferCylinder::RegisterObject(context);
        CollisionShape_Capsule::RegisterObject(context);
        CollisionShape_Cone::RegisterObject(context);
        CollisionShape_Geometry::RegisterObject(context);
        CollisionShape_ConvexHull::RegisterObject(context);
        CollisionShape_ConvexHullCompound::RegisterObject(context);
        CollisionShape_ConvexDecompositionCompound::RegisterObject(context);
        CollisionShape_TreeCollision::RegisterObject(context);
        CollisionShape_HeightmapTerrain::RegisterObject(context);

        RigidBody::RegisterObject(context);
        NewtonMeshObject::RegisterObject(context);
        Constraint::RegisterObject(context);
        FixedDistanceConstraint::RegisterObject(context);
        BallAndSocketConstraint::RegisterObject(context);
        SixDofConstraint::RegisterObject(context);
        HingeConstraint::RegisterObject(context);
        SliderConstraint::RegisterObject(context);
        FullyFixedConstraint::RegisterObject(context);
        KinematicsControllerConstraint::RegisterObject(context);
        RigidBodyContactEntry::RegisterObject(context);

    }












    RigidBodyContactEntry::RigidBodyContactEntry(Context* context) : Object(context)
    {

    }

    RigidBodyContactEntry::~RigidBodyContactEntry()
    {

    }

    void RigidBodyContactEntry::RegisterObject(Context* context)
    {
        context->RegisterFactory<RigidBodyContactEntry>();
    }

    void RigidBodyContactEntry::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        //draw contact points
        if (wakeFlag_)
        {
            for (int i = 0; i < numContacts; i++)
            {
                debug->AddLine(contactPositions[i], (contactPositions[i] + contactNormals[i]), Color::GREEN, depthTest);
            }
        }
    }



}
