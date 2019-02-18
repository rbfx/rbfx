#include "PhysicsWorld.h"
#include "Core/Profiler.h"
#include "RigidBody.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"
#include "UrhoNewtonConversions.h"
#include "IO/Log.h"
#include "Newton.h"

namespace Urho3D {

    void Newton_ApplyForceAndTorqueCallback(const NewtonBody* body, dFloat timestep, int threadIndex)
    {
        URHO3D_PROFILE_THREAD(NewtonThreadProfilerString(threadIndex).CString());
        URHO3D_PROFILE_FUNCTION()


        Vector3 netForce;
        Vector3 netTorque;
        RigidBody* rigidBodyComp = nullptr;

        rigidBodyComp = static_cast<RigidBody*>(NewtonBodyGetUserData(body));

        if (rigidBodyComp == nullptr)
            return;

        rigidBodyComp->GetForceAndTorque(netForce, netTorque);


        Vector3 gravityForce;
        if (rigidBodyComp->GetScene())//on scene destruction sometimes this is null so check...
        {
            PhysicsWorld* physicsWorld = rigidBodyComp->GetScene()->GetComponent<PhysicsWorld>();
            float physicsScale = physicsWorld->GetPhysicsScale();
            gravityForce = physicsWorld->GetGravity() * physicsScale* rigidBodyComp->GetEffectiveMass();


    
            netForce += gravityForce;



            //apply forces and torques scaled with the physics world scale accourdingly.
            NewtonBodySetForce(body, &UrhoToNewton(netForce*physicsScale*physicsScale*physicsScale)[0]);
            NewtonBodySetTorque(body, &UrhoToNewton(netTorque*physicsScale*physicsScale*physicsScale*physicsScale*physicsScale)[0]);

        }
    }


    void Newton_SetTransformCallback(const NewtonBody* body, const dFloat* matrix, int threadIndex)
    {
        RigidBody* rigBody = static_cast<RigidBody*>(NewtonBodyGetUserData(body));
        if(rigBody)
            rigBody->MarkInternalTransformDirty();
    }


    void Newton_DestroyBodyCallback(const NewtonBody* body)
    {

    }



    dFloat Newton_WorldRayCastFilterCallback(const NewtonBody* const body, const NewtonCollision* const collisionHit, const dFloat* const contact, const dFloat* const normal, dLong collisionID, void* const userData, dFloat intersetParam)
    {
        PhysicsRayCastUserData*  data = (PhysicsRayCastUserData*)userData;

        PhysicsRayCastIntersection intersection;
        intersection.body_ = (NewtonBody*)body;
        intersection.rayIntersectParameter_ = intersetParam;
        intersection.rayIntersectWorldPosition_ = NewtonToUrhoVec3(dVector(contact));
        intersection.rayIntersectWorldNormal_ = NewtonToUrhoVec3(dVector(normal));
        intersection.rigBody_ = (RigidBody*)NewtonBodyGetUserData(body);
        data->intersections += intersection;

        if (!data->singleIntersection_) {
            //continue
            return 1.0f;
        }
        else
            return 0.0f;
    }




    unsigned Newton_WorldRayPrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData)
    {
        ///no filtering right now.
        return 1;///?
    }




    void Newton_ProcessContactsCallback(const NewtonJoint* contactJoint, dFloat timestep, int threadIndex)
    {
        URHO3D_PROFILE_THREAD(NewtonThreadProfilerString(threadIndex).CString());
        URHO3D_PROFILE_FUNCTION();


        const NewtonBody* const body0 = NewtonJointGetBody0(contactJoint);
        const NewtonBody* const body1 = NewtonJointGetBody1(contactJoint);

        
        RigidBody* rigBody0 = static_cast<RigidBody*>(NewtonBodyGetUserData(body0));
        RigidBody* rigBody1 = static_cast<RigidBody*>(NewtonBodyGetUserData(body1));

        if (!rigBody0 || !rigBody1)
            return;


        unsigned int key = IntVector2(rigBody0->GetID(), rigBody1->GetID()).ToHash();

        PhysicsWorld* physicsWorld = rigBody0->GetPhysicsWorld();

        if (physicsWorld == nullptr)
            return;//scene is being destroyed.

        SharedPtr<RigidBodyContactEntry> contactEntry = nullptr;

        NewtonWorldCriticalSectionLock(physicsWorld->GetNewtonWorld(), threadIndex);
         
            contactEntry = rigBody0->GetCreateContactEntry(rigBody1);
        NewtonWorldCriticalSectionUnlock(physicsWorld->GetNewtonWorld());


        if (contactEntry->expired_) {
            contactEntry->body0 = rigBody0;
            contactEntry->body1 = rigBody1;

            contactEntry->expired_ = false;
            contactEntry->numContacts = 0;
        }

        if (NewtonJointIsActive(contactJoint))
            contactEntry->wakeFlag_ = NewtonJointIsActive(contactJoint);

        if (NewtonContactJointGetContactCount(contactJoint) > contactEntry->numContacts)
            contactEntry->numContacts = NewtonContactJointGetContactCount(contactJoint);



        if (contactEntry->numContacts > DEF_PHYSICS_MAX_CONTACT_POINTS)
        {
            URHO3D_LOGWARNING("Contact Entry Contact Count Greater Than DEF_PHYSICS_MAX_CONTACT_POINTS, consider increasing the limit.");
        }


        int contactIdx = 0;

        for (void* contact = NewtonContactJointGetFirstContact(contactJoint); contact; contact = NewtonContactJointGetNextContact(contactJoint, contact)) {


            NewtonMaterial* const material = NewtonContactGetMaterial(contact);

            NewtonCollision* shape0 = NewtonMaterialGetBodyCollidingShape(material, body0);
            NewtonCollision* shape1 = NewtonMaterialGetBodyCollidingShape(material, body1);


            CollisionShape* colShape0 = static_cast<CollisionShape*>(NewtonCollisionGetUserData(shape0));
            CollisionShape* colShape1 = static_cast<CollisionShape*>(NewtonCollisionGetUserData(shape1));




            {
                //get contact geometric info for the contact struct
                dVector pos, force, norm, tan0, tan1;
                NewtonMaterialGetContactPositionAndNormal(material, body0, &pos[0], &norm[0]);
                NewtonMaterialGetContactTangentDirections(material, body0, &tan0[0], &tan1[0]);
                NewtonMaterialGetContactForce(material, body0, &force[0]);


                contactEntry->contactNormals[contactIdx] = physicsWorld->PhysicsToScene_Domain(NewtonToUrhoVec3(norm));
                contactEntry->contactPositions[contactIdx] = physicsWorld->PhysicsToScene_Domain(NewtonToUrhoVec3(pos));
                contactEntry->contactTangent0[contactIdx] = physicsWorld->PhysicsToScene_Domain(NewtonToUrhoVec3(tan0));
                contactEntry->contactTangent1[contactIdx] = physicsWorld->PhysicsToScene_Domain(NewtonToUrhoVec3(tan1));
                contactEntry->contactForces[contactIdx] = physicsWorld->PhysicsToScene_Domain(NewtonToUrhoVec3(force));


                contactEntry->shapes0[contactIdx] = colShape0;
                contactEntry->shapes1[contactIdx] = colShape1;

                //#todo debugging
                //GetSubsystem<VisualDebugger>()->AddCross(contactEntry->contactPositions[contactIdx], 0.1f, Color::BLUE, true);

                contactIdx++;
            }


            float staticFriction0 = colShape0->GetStaticFriction();
            float kineticFriction0 = colShape0->GetKineticFriction();
            float elasticity0 = colShape0->GetElasticity();
            float softness0 = colShape0->GetSoftness();

            float staticFriction1 = colShape1->GetStaticFriction();
            float kineticFriction1 = colShape1->GetKineticFriction();
            float elasticity1 = colShape1->GetElasticity();
            float softness1 = colShape1->GetSoftness();


            float finalStaticFriction = Max(staticFriction0, staticFriction1);
            float finalKineticFriction = Max(kineticFriction0, kineticFriction1);
            float finalElasticity = Min(elasticity0, elasticity1);
            float finalSoftness = Max(softness0, softness1);

            //apply material settings to contact.
            NewtonMaterialSetContactFrictionCoef(material, finalStaticFriction, finalKineticFriction, 0);
            NewtonMaterialSetContactElasticity(material, finalElasticity);
            NewtonMaterialSetContactSoftness(material, finalSoftness);

            if (rigBody0->GetTriggerMode() || rigBody1->GetTriggerMode()) {
                NewtonContactJointRemoveContact(contactJoint, contact);
                continue;
            }
        }


    }






    int Newton_AABBOverlapCallback(const NewtonJoint* const contactJoint, dFloat timestep, int threadIndex)
    {
        URHO3D_PROFILE_THREAD(NewtonThreadProfilerString(threadIndex).CString());
        URHO3D_PROFILE_FUNCTION();


        const NewtonBody* const body0 = NewtonJointGetBody0(contactJoint);
        const NewtonBody* const body1 = NewtonJointGetBody1(contactJoint);

        RigidBody* rigBody0 = static_cast<RigidBody*>(NewtonBodyGetUserData(body0));
        RigidBody* rigBody1 = static_cast<RigidBody*>(NewtonBodyGetUserData(body1));

        if (!rigBody0 || !rigBody1)
            return 1;

        PhysicsWorld* physicsWorld = rigBody0->GetPhysicsWorld();

        if (physicsWorld == nullptr)
            return 1;//scene is being destroyed.

        //#todo why is this happening.
        if (!rigBody0->RefCountPtr() || !rigBody1->RefCountPtr())
        {
            return 1;
        }


        bool res;
        NewtonWorldCriticalSectionLock(physicsWorld->GetNewtonWorld(), threadIndex);

        res = rigBody1->CanCollideWith(rigBody0);


        NewtonWorldCriticalSectionUnlock(physicsWorld->GetNewtonWorld());
        return res;
    }


    int Newton_AABBCompoundOverlapCallback(const NewtonJoint* const contact, dFloat timestep, const NewtonBody* const body0, const void* const collisionNode0, const NewtonBody* const body1, const void* const collisionNode1, int threadIndex)
    {
        URHO3D_PROFILE_THREAD(NewtonThreadProfilerString(threadIndex).CString());
        URHO3D_PROFILE_FUNCTION();

        return 1;
    }

}
