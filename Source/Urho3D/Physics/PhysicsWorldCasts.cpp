#include "PhysicsWorld.h"
#include "Container/Vector.h"
#include "Math/Ray.h"
#include "UrhoNewtonConversions.h"
#include "dVector.h"
#include "Newton.h"
#include "Math/Matrix3x4.h"
#include "RigidBody.h"


namespace Urho3D {


    void PhysicsWorld::RayCast(Vector<PhysicsRayCastIntersection>& intersections, const Ray& ray, float maxDistance, unsigned maxBodyIntersections, unsigned collisionMask /*= M_MAX_UNSIGNED*/)
    {
        RayCast(intersections, ray.origin_, ray.origin_ + ray.direction_*maxDistance, maxBodyIntersections, collisionMask);
    }

    void PhysicsWorld::RayCast(Vector<PhysicsRayCastIntersection>& intersections, const Vector3& pointOrigin, const Vector3& pointDestination, unsigned maxBodyIntersections /*= M_MAX_UNSIGNED*/, unsigned collisionMask /*= M_MAX_UNSIGNED*/)
    {
        WaitForUpdateFinished();

        intersections.Clear();
        PhysicsRayCastUserData data;

        Vector3 origin = (pointOrigin);
        Vector3 destination = (pointDestination);
        Vector3 direction = (pointDestination - pointOrigin).Normalized();


        data.bodyIntersectionCounter_ = maxBodyIntersections;

        NewtonWorldRayCast(newtonWorld_, &UrhoToNewton(origin)[0], &UrhoToNewton(destination)[0], Newton_WorldRayCastFilterCallback, &data, NULL, 0);



        int initialSize = data.intersections.Size();
        for (int i = 0; i < initialSize; i++)
        {
            PhysicsRayCastIntersection& intersection = data.intersections[i];
            unsigned collisionLayerAsBit = CollisionLayerAsBit(intersection.rigBody_->GetCollisionLayer());


            if ((collisionLayerAsBit & collisionMask)) {

                intersection.rayIntersectWorldPosition_ = (intersection.rayIntersectWorldPosition_);
                intersection.rayOriginWorld_ = pointOrigin;
                intersection.rayDistance_ = (intersection.rayIntersectWorldPosition_ - pointOrigin).Length();

                //Get Intersections with sub Collisions
                if (intersection.rigBody_) {

                    Matrix3x4 bodyTransform = intersection.rigBody_->GetWorldTransform();



                    Vector3 rayOriginLocal = bodyTransform.Inverse() * origin;
                    Vector3 rayDestinationLocal = bodyTransform.Inverse() * destination;
                    Vector3 rayDirLocal = bodyTransform.RotationMatrix().Inverse() * direction;

                    NewtonCollision* const compoundCollision = NewtonBodyGetCollision(intersection.body_);
                    if (NewtonCollisionGetType(compoundCollision) == SERIALIZE_ID_COMPOUND)
                    {
                        // we found a compound, find all sub shape on the path of the ray

                        for (void* node = NewtonCompoundCollisionGetFirstNode(compoundCollision); node; node = NewtonCompoundCollisionGetNextNode(compoundCollision, node)) {

                            dVector normal;
                            dLong attribute;
                            NewtonCollision* const subShape = NewtonCompoundCollisionGetCollisionFromNode(compoundCollision, node);
                            dFloat t = NewtonCollisionRayCast(subShape, &UrhoToNewton(rayOriginLocal)[0], &UrhoToNewton(rayDestinationLocal)[0], &normal[0], &attribute);
                            if (t <= 1.0f) {

                                float tWorld = t * (rayDestinationLocal - rayOriginLocal).Length();

                                PhysicsRayCastIntersection newIntersection;
                                newIntersection.body_ = intersection.body_;
                                newIntersection.rigBody_ = intersection.rigBody_;
                                newIntersection.collision_ = compoundCollision;
                                newIntersection.subCollision_ = subShape;
                                newIntersection.rayIntersectParameter_ = t;
                                newIntersection.rayDistance_ = intersection.rayDistance_ + tWorld;
                                newIntersection.rayOriginWorld_ = intersection.rayOriginWorld_;
                                newIntersection.rayIntersectWorldNormal_ = bodyTransform.RotationMatrix() * NewtonToUrhoVec3(normal);
                                newIntersection.rayIntersectWorldPosition_ = bodyTransform * (rayOriginLocal + rayDirLocal * tWorld);

                                data.intersections.Push(newIntersection);
                            }
                        }
                    }

                }
            }
        }


        //sort the intersections by distance. we do this because the order that you get is based off bounding box intersection and that is not nessecarily the same of surface intersection order.
        if (data.intersections.Size() > 1)
            Sort(data.intersections.Begin(), data.intersections.End(), PhysicsRayCastIntersectionCompare);

        intersections = data.intersections;
    }

}
