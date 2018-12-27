//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>

#include "CreateRagdoll.h"

#include <Urho3D/DebugNew.h>
#include "Urho3D/Physics/CollisionShapesDerived.h"
#include "Urho3D/Physics/BallAndSocketConstraint.h"
#include "Urho3D/Physics/HingeConstraint.h"
#include "Urho3D/Physics/FixedDistanceConstraint.h"
#include "Urho3D/Physics/FullyFixedConstraint.h"

CreateRagdoll::CreateRagdoll(Context* context) :
    Component(context)
{
}

void CreateRagdoll::OnNodeSet(Node* node)
{
    // If the node pointer is non-null, this component has been created into a scene node. Subscribe to physics collisions that
    // concern this scene node
    if (node)
        SubscribeToEvent(node, E_NODECOLLISION, URHO3D_HANDLER(CreateRagdoll, HandleNodeCollision));
}

void CreateRagdoll::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollision;

    // Get the other colliding body, make sure it is moving (has nonzero mass)
    auto* otherBody = static_cast<RigidBody*>(eventData[P_OTHERBODY].GetPtr());

    if (otherBody->GetEffectiveMass() > 0.0f)
    {
        // We do not need the physics components in the AnimatedModel's root scene node anymore
        node_->RemoveComponent<RigidBody>();
        node_->RemoveComponent<CollisionShape>();

        // Create RigidBody & CollisionShape components to bones
        CreateRagdollBone("Bip01_Pelvis", CollisionShape_Box::GetTypeNameStatic(), Vector3(0.3f, 0.2f, 0.25f), Vector3(0.0f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 0.0f));
        CreateRagdollBone("Bip01_Spine1", CollisionShape_Box::GetTypeNameStatic(), Vector3(0.35f, 0.2f, 0.3f), Vector3(0.15f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 0.0f));
        CreateRagdollBone("Bip01_L_Thigh", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.175f, 0.45f, 0.175f), Vector3(0.25f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_R_Thigh", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.175f, 0.45f, 0.175f), Vector3(0.25f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_L_Calf", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.15f, 0.55f, 0.15f), Vector3(0.25f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_R_Calf", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.15f, 0.55f, 0.15f), Vector3(0.25f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_Head", CollisionShape_Box::GetTypeNameStatic(), Vector3(0.2f, 0.2f, 0.2f), Vector3(0.1f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 0.0f));
        CreateRagdollBone("Bip01_L_UpperArm", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.15f, 0.35f, 0.15f), Vector3(0.1f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_R_UpperArm", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.15f, 0.35f, 0.15f), Vector3(0.1f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_L_Forearm", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.125f, 0.4f, 0.125f), Vector3(0.2f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));
        CreateRagdollBone("Bip01_R_Forearm", CollisionShape_Capsule::GetTypeNameStatic(), Vector3(0.125f, 0.4f, 0.125f), Vector3(0.2f, 0.0f, 0.0f),
            Quaternion(0.0f, 0.0f, 90.0f));

        // Create Constraints between bones

        Quaternion jointOrientation;

        jointOrientation = Quaternion::IDENTITY;
        jointOrientation = Quaternion(0, 0, -75) * jointOrientation;
        CreateRagdollConstraint("Bip01_L_Thigh", "Bip01_Pelvis", BallAndSocketConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(45.0f, 45.0f), Vector2(-20,20));


        jointOrientation = Quaternion::IDENTITY;
        jointOrientation = Quaternion(0, 0, 180+75 ) * jointOrientation;
        CreateRagdollConstraint("Bip01_R_Thigh", "Bip01_Pelvis", BallAndSocketConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(45.0f, 45.0f), Vector2(-20, 20));



        jointOrientation = Quaternion::IDENTITY;
        CreateRagdollConstraint("Bip01_L_Calf", "Bip01_L_Thigh", HingeConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(-90.0f, 0.0f), Vector2::ZERO);


        jointOrientation = Quaternion::IDENTITY;
        CreateRagdollConstraint("Bip01_R_Calf", "Bip01_R_Thigh", HingeConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(-90.0f, 0.0f), Vector2::ZERO);


        jointOrientation = Quaternion::IDENTITY;
        CreateRagdollConstraint("Bip01_Spine1", "Bip01_Pelvis", HingeConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(-45.0f, 45.0f), Vector2::ZERO);



        jointOrientation = Quaternion::IDENTITY;
        jointOrientation.FromAxes(Vector3::UP, Vector3::LEFT, Vector3::FORWARD);
        jointOrientation = Quaternion(-15, 0, 0) * jointOrientation ;
        CreateRagdollConstraint("Bip01_Head", "Bip01_Spine1", BallAndSocketConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(30.0f, 30.0f), Vector2(-20, 20));

        jointOrientation = Quaternion::IDENTITY;
        jointOrientation = Quaternion(0, 0, -50) * jointOrientation;
        CreateRagdollConstraint("Bip01_L_UpperArm", "Bip01_Spine1", BallAndSocketConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(45.0f, 45.0f), Vector2(-20, 20), false);

        jointOrientation = Quaternion::IDENTITY;
        jointOrientation = Quaternion(0, 0, 50+180) * jointOrientation;
        CreateRagdollConstraint("Bip01_R_UpperArm", "Bip01_Spine1", BallAndSocketConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(45.0f, 45.0f), Vector2(-20, 20), false);

        jointOrientation = Quaternion::IDENTITY;
        CreateRagdollConstraint("Bip01_L_Forearm", "Bip01_L_UpperArm", HingeConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(0.0f, 45.0f), Vector2::ZERO);

        jointOrientation = Quaternion::IDENTITY;
        CreateRagdollConstraint("Bip01_R_Forearm", "Bip01_R_UpperArm", HingeConstraint::GetTypeNameStatic(), jointOrientation,
            Vector2(0.0f, 45.0f), Vector2::ZERO);

        // Disable keyframe animation from all bones so that they will not interfere with the ragdoll
        auto* model = GetComponent<AnimatedModel>();
        Skeleton& skeleton = model->GetSkeleton();
        for (unsigned i = 0; i < skeleton.GetNumBones(); ++i)
            skeleton.GetBone(i)->animated_ = false;

        // Finally remove self from the scene node. Note that this must be the last operation performed in the function
        Remove();
    }
}

void CreateRagdoll::CreateRagdollBone(const String& boneName, StringHash collisionShapeType, const Vector3& size, const Vector3& position,
    const Quaternion& rotation)
{
    // Find the correct child scene node recursively
    Node* boneNode = node_->GetChild(boneName, true);
    if (!boneNode)
    {
        URHO3D_LOGWARNING("Could not find bone " + boneName + " for creating ragdoll physics components");
        return;
    }

    auto* body = boneNode->CreateComponent<RigidBody>();
    // Set mass to make movable
    body->SetMassScale(1.0f);
    // Set damping parameters to smooth out the motion
    body->SetLinearDamping(1.0f);
    body->SetAngularDamping(.85f);
    body->SetInterpolationFactor(0.3f);
    

    CollisionShape* shape = nullptr;
    // We use either a box or a capsule shape for all of the bones
    if (collisionShapeType == CollisionShape_Box::GetTypeStatic()) {
        shape = boneNode->CreateComponent<CollisionShape_Box>();
        shape->SetPositionOffset(position);
        shape->SetRotationOffset(rotation);
        shape->SetScaleFactor(size);
    }
    else {

        shape = boneNode->CreateComponent<CollisionShape_Capsule>();
        shape->SetPositionOffset(position);
        shape->SetRotationOffset(Quaternion(90, 0, 0));
        static_cast<CollisionShape_Capsule*>(shape)->SetLength(size.y_);
        static_cast<CollisionShape_Capsule*>(shape)->SetRadius1(size.x_*0.5f);
        static_cast<CollisionShape_Capsule*>(shape)->SetRadius2(size.x_*0.5f);
    }
}

void CreateRagdoll::CreateRagdollConstraint(const String& boneName, const String& parentName, StringHash constraintType,
    const Quaternion& orientation, const Vector2& angleLimits, const Vector2& twistLimits,
    bool disableCollision)
{
    Node* boneNode = node_->GetChild(boneName, true);
    Node* parentNode = node_->GetChild(parentName, true);
    if (!boneNode)
    {
        URHO3D_LOGWARNING("Could not find bone " + boneName + " for creating ragdoll constraint");
        return;
    }
    if (!parentNode)
    {
        URHO3D_LOGWARNING("Could not find bone " + parentName + " for creating ragdoll constraint");
        return;
    }

    Constraint* constraint = nullptr;
    if (constraintType == BallAndSocketConstraint::GetTypeNameStatic()) {
        constraint = boneNode->CreateComponent<BallAndSocketConstraint>();
        // The connected body must be specified before setting the world position
        constraint->SetOtherBody(parentNode->GetComponent<RigidBody>());

        static_cast<BallAndSocketConstraint*>(constraint)->SetConeAngle(Max(angleLimits.x_, angleLimits.y_)*0.5f);
        static_cast<BallAndSocketConstraint*>(constraint)->SetTwistLimitsEnabled(true);
        static_cast<BallAndSocketConstraint*>(constraint)->SetTwistLimits(twistLimits.x_, twistLimits.y_);
        static_cast<BallAndSocketConstraint*>(constraint)->SetWorldRotation(orientation);

        //static_cast<BallAndSocketConstraint*>(constraint)->SetConeFriction(0.01f);
    }
    else if (constraintType == HingeConstraint::GetTypeNameStatic())
    {


        constraint = boneNode->CreateComponent<HingeConstraint>();

        // The connected body must be specified before setting the world position
        constraint->SetOtherBody(parentNode->GetComponent<RigidBody>());

        static_cast<HingeConstraint*>(constraint)->SetMinAngle(angleLimits.x_);
        static_cast<HingeConstraint*>(constraint)->SetMaxAngle(angleLimits.y_);
        static_cast<HingeConstraint*>(constraint)->SetWorldRotation(orientation);
    }


    // Most of the constraints in the ragdoll will work better when the connected bodies don't collide against each other
    constraint->SetDisableCollision(disableCollision);


    // Position the constraint at the child bone we are connecting
    constraint->SetWorldPosition(boneNode->GetWorldPosition());

    //ragdolls are soft things - so loosen up the constraints purposefully.
    constraint->SetSolveMode(SOLVE_MODE_ITERATIVE);
    constraint->SetStiffness(0.0f);
    

}
