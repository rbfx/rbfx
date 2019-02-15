#include "PhysicsWorld.h"
#include "Math/Sphere.h"
#include "UrhoNewtonConversions.h"
#include "dMatrix.h"
#include "RigidBody.h"
#include "CollisionShape.h"
#include "IO/Log.h"

namespace Urho3D {

    bool PhysicsWorld::RigidBodyContainsPoint(RigidBody* rigidBody, const Vector3&worldPoint)
    {
        dVector contact;
        dVector normal;

        NewtonCollision* effectiveCollision = rigidBody->GetEffectiveNewtonCollision();

        if (!effectiveCollision)
            return false;

        dMatrix collisionMatrix;
        NewtonCollisionGetMatrix(effectiveCollision, &collisionMatrix[0][0]);

        //#todo double check the matrix here may need tweeking to get the true transform of the compound:
        int res = NewtonCollisionPointDistance(newtonWorld_,
            &UrhoToNewton(worldPoint)[0],
            rigidBody->GetEffectiveNewtonCollision(),
            &UrhoToNewton(rigidBody->GetPhysicsTransform(true) * Matrix3x4(NewtonToUrhoMat4(collisionMatrix)))[0][0], &contact[0], &normal[0], 0);


        return !res;
    }

    void PhysicsWorld::GetRigidBodies(PODVector<RigidBody*>& result, const Sphere& sphere, unsigned collisionMask /*= M_MAX_UNSIGNED*/)
    {
        //#todo use collision mask

        Matrix3x4 mat;
        mat.SetTranslation(sphere.center_);

        NewtonCollision* newtonShape = UrhoShapeToNewtonCollision(newtonWorld_, sphere, false);
        int numContacts = DoNewtonCollideTest(&UrhoToNewton(mat)[0][0], newtonShape);

        GetBodiesInConvexCast(result, numContacts);

        NewtonDestroyCollision(newtonShape);
    }

    void PhysicsWorld::GetRigidBodies(PODVector<RigidBody*>& result, const BoundingBox& box, unsigned collisionMask /*= M_MAX_UNSIGNED*/)
    {
        //#todo use collision mask
        Matrix3x4 mat;
        mat.SetTranslation(box.Center());

        NewtonCollision* newtonShape = UrhoShapeToNewtonCollision(newtonWorld_, box, false);
        int numContacts = DoNewtonCollideTest(&UrhoToNewton(mat)[0][0], newtonShape);

        GetBodiesInConvexCast(result, numContacts);
        NewtonDestroyCollision(newtonShape);

    }


    void PhysicsWorld::GetRigidBodies(PODVector<RigidBody*>& result, const RigidBody* body)
    {
        dMatrix mat;
        NewtonBodyGetMatrix(body->GetNewtonBody(), &mat[0][0]);
        NewtonCollision* newtonShape = body->GetEffectiveNewtonCollision();
        int numContacts = DoNewtonCollideTest(&mat[0][0], newtonShape);

        GetBodiesInConvexCast(result, numContacts);
    }

    int PhysicsWorld::DoNewtonCollideTest(const dFloat* const matrix, const NewtonCollision* shape)
    {
        URHO3D_LOGINFO("DoNewtonCollideTest needs more testing.");
        return  NewtonWorldCollide(newtonWorld_,
            matrix, shape, nullptr,
            Newton_WorldRayPrefilterCallback, convexCastRetInfoArray,
            convexCastRetInfoSize_, 0);

    }
    void PhysicsWorld::GetBodiesInConvexCast(PODVector<RigidBody*>& result, int numContacts)
    {
        //iterate though contacts.
        for (int i = 0; i < numContacts; i++) {

            if (convexCastRetInfoArray[i].m_hitBody != nullptr) {

                void* userData = NewtonBodyGetUserData(convexCastRetInfoArray[i].m_hitBody);
                if (userData != nullptr)
                {
                    RigidBody* rigBody = static_cast<RigidBody*>(userData);
                    result.Push(rigBody);
                }
            }
        }
    }
}

