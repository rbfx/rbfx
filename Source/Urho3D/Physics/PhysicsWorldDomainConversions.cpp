#include "PhysicsWorld.h"



namespace Urho3D
{

    float PhysicsWorld::SceneToPhysics_Domain(float x)
    {
        return x * physicsScale_;
    }


    Urho3D::Vector3 PhysicsWorld::SceneToPhysics_Domain(Vector3 v)
    {
        return v * physicsScale_;
    }


    Urho3D::Matrix3x4 PhysicsWorld::SceneToPhysics_Domain(Matrix3x4 sceneFrame)
    {
        return GetPhysicsWorldFrame().Inverse() * sceneFrame;
    }


    Vector3 PhysicsWorld::SceneToPhysics_Domain_Torque(Vector3 torque)
    {
        return torque * physicsScale_*physicsScale_*physicsScale_*physicsScale_*physicsScale_;
    }
    float PhysicsWorld::SceneToPhysics_Domain_Torque(float torque)
    {
        return torque * physicsScale_*physicsScale_*physicsScale_*physicsScale_*physicsScale_;
    }



    float PhysicsWorld::PhysicsToScene_Domain(float x)
    {
        return x / physicsScale_;
    }


    Urho3D::Vector3 PhysicsWorld::PhysicsToScene_Domain(Vector3 v)
    {
        return v / physicsScale_;
    }


    Urho3D::Matrix3x4 PhysicsWorld::PhysicsToScene_Domain(Matrix3x4 newtonFrame)
    {
        return GetPhysicsWorldFrame() * newtonFrame;
    }


    Vector3 PhysicsWorld::PhysicsToScene_Domain_Torque(Vector3 torque)
    {
        return torque / (physicsScale_*physicsScale_*physicsScale_*physicsScale_*physicsScale_);

    }
    float PhysicsWorld::PhysicsToScene_Domain_Torque(float torque)
    {
        return torque / (physicsScale_*physicsScale_*physicsScale_*physicsScale_*physicsScale_);
    }
}
