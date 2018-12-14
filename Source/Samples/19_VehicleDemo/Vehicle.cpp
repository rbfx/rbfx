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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

#include "Vehicle.h"
#include "Urho3D/Physics/CollisionShapesDerived.h"
#include "Urho3D/Physics/HingeConstraint.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Physics/SliderConstraint.h"
#include "Urho3D/SystemUI/SystemUI.h"

Vehicle::Vehicle(Context* context) :
    LogicComponent(context)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Vehicle::RegisterObject(Context* context)
{
    context->RegisterFactory<Vehicle>();

    URHO3D_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Steering", float, steering_, 0.0f, AM_DEFAULT);
    // Register wheel node IDs as attributes so that the wheel nodes can be reaquired on deserialization. They need to be tagged
    // as node ID's so that the deserialization code knows to rewrite the IDs in case they are different on load than on save
    URHO3D_ATTRIBUTE("Front Left Node", unsigned, frontLeftID_, 0, AM_DEFAULT | AM_NODEID);
    URHO3D_ATTRIBUTE("Front Right Node", unsigned, frontRightID_, 0, AM_DEFAULT | AM_NODEID);
    URHO3D_ATTRIBUTE("Rear Left Node", unsigned, rearLeftID_, 0, AM_DEFAULT | AM_NODEID);
    URHO3D_ATTRIBUTE("Rear Right Node", unsigned, rearRightID_, 0, AM_DEFAULT | AM_NODEID);
}

void Vehicle::ApplyAttributes()
{
    // This function is called on each Serializable after the whole scene has been loaded. Reacquire wheel nodes from ID's
    // as well as all required physics components
    Scene* scene = GetScene();

    frontLeft_ = scene->GetNode(frontLeftID_);
    frontRight_ = scene->GetNode(frontRightID_);
    rearLeft_ = scene->GetNode(rearLeftID_);
    rearRight_ = scene->GetNode(rearRightID_);
    hullBody_ = node_->GetComponent<RigidBody>();

    GetWheelComponents();
}

void Vehicle::FixedUpdate(float timeStep)
{
    float newSteering = 0.0f;
    float accelerator = 0.0f;

    // Read controls
    if (controls_.buttons_ & CTRL_LEFT)
        newSteering = -1.0f;
    if (controls_.buttons_ & CTRL_RIGHT)
        newSteering = 1.0f;
    if (controls_.buttons_ & CTRL_FORWARD)
        accelerator = 1.0f;
    if (controls_.buttons_ & CTRL_BACK)
        accelerator = -0.5f;

    steering_ = newSteering;

    frontLeftSteeringAxis_->SetActuatorTargetAngle(steering_ * MAX_WHEEL_ANGLE);
    frontRightSteeringAxis_->SetActuatorTargetAngle(steering_ * MAX_WHEEL_ANGLE);


    static_cast<HingeConstraint*>(rearRightAxis_.Get())->SetMotorTargetAngularRate(MAX_SPEED * accelerator);
    static_cast<HingeConstraint*>(rearLeftAxis_.Get())->SetMotorTargetAngularRate(MAX_SPEED * accelerator);
    static_cast<HingeConstraint*>(frontRightAxis_.Get())->SetMotorTargetAngularRate(MAX_SPEED * accelerator);
    static_cast<HingeConstraint*>(frontLeftAxis_.Get())->SetMotorTargetAngularRate(MAX_SPEED * accelerator);






}

void Vehicle::Init()
{
    // This function is called only from the main program when initially creating the vehicle, not on scene load
    auto* cache = GetSubsystem<ResourceCache>();

    auto* hullObject = node_->CreateComponent<StaticModel>();
    hullBody_ = node_->CreateComponent<RigidBody>();
    auto* hullShape = node_->CreateComponent<CollisionShape_Box>();

    node_->SetScale(Vector3(1.5f, 1.0f, 3.0f));
    hullObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    hullObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    hullObject->SetCastShadows(true);
    hullBody_->SetMassScale(1.0f);

    InitWheel("FrontLeft", Vector3(-0.6f, -0.4f*2.0f, 0.3f), frontLeft_, frontLeftID_, true);
    InitWheel("FrontRight", Vector3(0.6f, -0.4f*2.0f, 0.3f), frontRight_, frontRightID_, true);
    InitWheel("RearLeft", Vector3(-0.6f, -0.4f*2.0f, -0.3f), rearLeft_, rearLeftID_, false);
    InitWheel("RearRight", Vector3(0.6f, -0.4f*2.0f, -0.3f), rearRight_, rearRightID_, false);

    GetWheelComponents();
}

void Vehicle::InitWheel(const String& name, const Vector3& offset, WeakPtr<Node>& wheelNode, unsigned& wheelNodeID, bool isSteering)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Note: do not parent the wheel to the hull scene node. Instead create it on the root level and let the physics
    // constraint keep it together
    wheelNode = GetScene()->CreateChild(name);
    wheelNode->SetPosition(node_->LocalToWorld(offset) );
    wheelNode->SetRotation(node_->GetRotation() * (offset.x_ >= 0.0 ? Quaternion(0.0f, 0.0f, -90.0f) :
        Quaternion(0.0f, 0.0f, 90.0f)));
    wheelNode->SetScale(Vector3(0.8f, 0.5f, 0.8f));
    // Remember the ID for serialization
    wheelNodeID = wheelNode->GetID();

    auto* wheelObject = wheelNode->CreateComponent<StaticModel>();
    auto* wheelBody = wheelNode->CreateComponent<RigidBody>();
    auto* wheelShape = wheelNode->CreateComponent<CollisionShape_Sphere>();

    wheelObject->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
    wheelObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    wheelObject->SetCastShadows(true);
    wheelBody->SetMassScale(1.0f);
    wheelBody->SetCollisionOverride(GetComponent<RigidBody>(), false);


    wheelShape->SetRotationOffset(Quaternion(0, 0, 90));
    wheelShape->SetInheritNodeScale(false);
    wheelShape->SetScaleFactor(Vector3(0.8f, 0.8, 0.8f));







    //make hubs where wheels are.
    auto* hubNode = GetScene()->CreateChild(name + "Hub");
    hubNode->SetWorldPosition(wheelNode->GetWorldPosition());
    auto* hubBody = hubNode->CreateComponent<RigidBody>();
    hubBody->SetNoCollideOverride(true);
    auto* hubShape = hubNode->CreateComponent<CollisionShape_Box>();
    hubShape->SetScaleFactor(0.5f);


    //connect hubs to main body with suspension.
    auto* hubSuspension = node_->CreateComponent<SliderConstraint>();
    hubSuspension->SetOtherBody(hubBody);
    hubSuspension->SetWorldPosition(wheelNode->GetWorldPosition());
    hubSuspension->SetWorldRotation(Quaternion(0, 0, 90));
    hubSuspension->SetEnableSliderSpringDamper(true);
    hubSuspension->SetSliderSpringCoefficient(700);
    hubSuspension->SetSliderDamperCoefficient(70);
    hubSuspension->SetEnableTwistLimits(true, true);




    //for front tires create a 
    Node* steeringNode = nullptr;
    if (isSteering)
    {
        //make a secondary small body that is attached with a hinge actuator to the main vehicle body, then attach the wheel to the secondary body.
        steeringNode = GetScene()->CreateChild(name + "Steering");
        auto* steeringBody = steeringNode->CreateComponent<RigidBody>();
        auto steeringShape = steeringNode->CreateComponent<CollisionShape_Box>();
        steeringShape->SetScaleFactor(0.25f);//make the body small bug still visible.


        steeringNode->SetPosition(wheelNode->GetWorldPosition());
        steeringBody->SetNoCollideOverride(true);
        

        auto steeringConstraint = steeringNode->CreateComponent<HingeConstraint>();

        steeringConstraint->SetPowerMode(HingeConstraint::ACTUATOR);
        steeringConstraint->SetActuatorMaxAngularRate(10.0f);
        steeringConstraint->SetMaxTorque(100.0f);
        steeringConstraint->SetMaxAngle(MAX_WHEEL_ANGLE);
        steeringConstraint->SetMinAngle(-MAX_WHEEL_ANGLE);
        steeringConstraint->SetOtherBody(hubBody);
        steeringConstraint->SetWorldRotation(Quaternion(0, 0, 90));
    }











    auto* wheelConstraint = wheelNode->CreateComponent<HingeConstraint>();
    if(isSteering)
        wheelConstraint->SetOtherBody(steeringNode->GetComponent<RigidBody>()); // Connect to the steering body
    else
        wheelConstraint->SetOtherBody(hubBody); // Connect to the hub node.


    wheelConstraint->SetWorldPosition(wheelNode->GetWorldPosition());
    wheelConstraint->SetWorldRotation(Quaternion(0, 0, 0));










    wheelConstraint->SetWorldPosition(wheelNode->GetWorldPosition() ); // Set constraint's both ends at wheel's location
    //wheelConstraint->SetWorldRotation(Quaternion(0,0,-90));
    wheelConstraint->SetEnableLimits(false);//allow free spin
    wheelConstraint->SetDisableCollision(true); // Let the wheel intersect the vehicle hull
    wheelConstraint->SetPowerMode(HingeConstraint::MOTOR); // Make the constraint powered.
    wheelConstraint->SetMotorTargetAngularRate(0.0f); //With zero speed
    wheelConstraint->SetMaxTorque(ENGINE_POWER); //specify max torque the motor can exert to reach desired angular rate.
}

void Vehicle::GetWheelComponents()
{
    frontLeftAxis_ = frontLeft_->GetDerivedComponent<Constraint>();
    frontRightAxis_ = frontRight_->GetDerivedComponent<Constraint>();
    rearLeftAxis_ = rearLeft_->GetDerivedComponent<Constraint>();
    rearRightAxis_ = rearRight_->GetDerivedComponent<Constraint>();

    frontLeftBody_ = frontLeft_->GetComponent<RigidBody>();
    frontRightBody_ = frontRight_->GetComponent<RigidBody>();
    rearLeftBody_ = rearLeft_->GetComponent<RigidBody>();
    rearRightBody_ = rearRight_->GetComponent<RigidBody>();

    frontLeftSteeringAxis_ = GetScene()->GetChild(frontLeft_->GetName() + "Steering", true)->GetComponent<HingeConstraint>();
    frontRightSteeringAxis_ = GetScene()->GetChild(frontRight_->GetName() + "Steering", true)->GetComponent<HingeConstraint>();
}
