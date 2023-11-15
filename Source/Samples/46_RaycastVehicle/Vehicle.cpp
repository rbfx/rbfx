//
// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Vehicle.h"
#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/RaycastVehicle.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;

const float CHASSIS_WIDTH = 2.6f;

void Vehicle2::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Vehicle2>();

    URHO3D_ATTRIBUTE("Steering", float, steering_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Controls Yaw", GetYaw, SetYaw, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Controls Pitch", GetPitch, SetPitch, float, 0.0f, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
        "Input Map", GetInputMapAttr, SetInputMapAttr, ResourceRef, ResourceRef(InputMap::GetTypeStatic()), AM_DEFAULT);
}

Vehicle2::Vehicle2(Urho3D::Context* context)
    : BaseClassName(context)
    , steering_(0.0f)
{
    SetUpdateEventMask(USE_FIXEDUPDATE | USE_POSTUPDATE);
    wheelRadius_ = 0.5f;
    suspensionRestLength_ = 0.6f;
    wheelWidth_ = 0.4f;
    suspensionStiffness_ = 14.0f;
    suspensionDamping_ = 2.0f;
    suspensionCompression_ = 4.0f;
    wheelFriction_ = 1000.0f;
    rollInfluence_ = 0.12f;
    emittersCreated = false;
}

Vehicle2::~Vehicle2() = default;

void Vehicle2::Init()
{
    auto* vehicle = node_->CreateComponent<RaycastVehicle>();
    vehicle->Init();
    auto* hullBody = node_->GetComponent<RigidBody>();
    hullBody->SetMass(800.0f);
    hullBody->SetLinearDamping(0.2f); // Some air resistance
    hullBody->SetAngularDamping(0.5f);
    hullBody->SetCollisionLayer(1);
    // This function is called only from the main program when initially creating the vehicle, not on scene load
    auto* cache = GetSubsystem<ResourceCache>();
    // Setting-up collision shape
    auto* hullColShape = node_->CreateComponent<CollisionShape>();
    Vector3 v3BoxExtents = Vector3(2.3f, 1.0f, 4.0f);
    hullColShape->SetBox(v3BoxExtents);
    auto* box = node_->CreateChild();
    auto* hullObject = box->CreateComponent<StaticModel>();
    box->SetScale(v3BoxExtents);
    hullObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    hullObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    hullObject->SetCastShadows(true);
    float connectionHeight = -0.4f;
    bool isFrontWheel = true;
    Vector3 wheelDirection(0, -1, 0);
    Vector3 wheelAxle(-1, 0, 0);
    // We use not scaled coordinates here as everything will be scaled.
    // Wheels are on bottom at edges of the chassis
    // Note we don't set wheel nodes as children of hull (while we could) to avoid scaling to affect them.
    float wheelX = CHASSIS_WIDTH / 2.0f - wheelWidth_;
    // Front left
    connectionPoints_[0] = Vector3(-wheelX, connectionHeight, 2.5f - GetWheelRadius() * 2.0f);
    // Front right
    connectionPoints_[1] = Vector3(wheelX, connectionHeight, 2.5f - GetWheelRadius() * 2.0f);
    // Back left
    connectionPoints_[2] = Vector3(-wheelX, connectionHeight, -2.5f + GetWheelRadius() * 2.0f);
    // Back right
    connectionPoints_[3] = Vector3(wheelX, connectionHeight, -2.5f + GetWheelRadius() * 2.0f);
    const Color LtBrown(0.972f, 0.780f, 0.412f);
    for (int id = 0; id < sizeof(connectionPoints_) / sizeof(connectionPoints_[0]); id++)
    {
        Node* wheelNode = node_->CreateChild();
        Vector3 connectionPoint = connectionPoints_[id];
        // Front wheels are at front (z > 0)
        // back wheels are at z < 0
        // Setting rotation according to wheel position
        bool isFrontWheel = connectionPoints_[id].z_ > 0.0f;
        auto rot = connectionPoint.x_ >= 0.0 ? Quaternion(0.0f, 0.0f, -90.0f) : Quaternion(0.0f, 0.0f, 90.0f);
        wheelNode->SetRotation(rot);
        wheelNode->SetWorldPosition(node_->GetWorldPosition() + node_->GetWorldRotation() * connectionPoints_[id]);
        auto* wheel = wheelNode->GetOrCreateComponent<RaycastVehicleWheel>();
        wheel->SetConnectionPoint(connectionPoints_[id]);
        wheel->SetDirection(wheelDirection);
        wheel->SetRotation(rot);
        wheel->SetAxle(wheelAxle);
        wheel->SetSuspensionRestLength(suspensionRestLength_);
        wheel->SetRadius(wheelRadius_);
        if (isFrontWheel)
        {
            wheel->SetSteeringFactor(1.0f);
            wheel->SetEngineFactor(0.0f);
        }
        else
        {
            wheel->SetSteeringFactor(0.0f);
            wheel->SetEngineFactor(1.0f);
        }
        wheel->SetSuspensionStiffness(suspensionStiffness_);
        wheel->SetDampingRelaxation(suspensionDamping_);
        wheel->SetDampingCompression(suspensionCompression_);
        wheel->SetFrictionSlip(wheelFriction_);
        wheel->SetRollInfluence(rollInfluence_);
        wheelNode->SetScale(Vector3(1.0f, 0.65f, 1.0f));
        auto* pWheel = wheelNode->CreateComponent<StaticModel>();
        pWheel->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
        pWheel->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        pWheel->SetCastShadows(true);
        CreateEmitter(connectionPoints_[id]);
    }
    emittersCreated = true;
    vehicle->ResetWheels();
}

void Vehicle2::SetInputMap(InputMap* inputMap)
{
    inputMap_ = inputMap;
}

void Vehicle2::SetInputMapAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetInputMap(cache->GetResource<InputMap>(value.name_));
}

ResourceRef Vehicle2::GetInputMapAttr() const
{
    return GetResourceRef(inputMap_, InputMap::GetTypeStatic());
}

void Vehicle2::CreateEmitter(Vector3 place)
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* emitter = GetScene()->CreateChild();
    emitter->SetWorldPosition(node_->GetWorldPosition() + node_->GetWorldRotation() * place + Vector3(0, -wheelRadius_, 0));
    auto* particleEmitter = emitter->CreateComponent<ParticleEmitter>();
    particleEmitter->SetEffect(cache->GetResource<ParticleEffect>("Particle/Dust.xml"));
    particleEmitter->SetEmitting(false);
    particleEmitterNodeList_.push_back(emitter);
    emitter->SetTemporary(true);
}

/// Applying attributes
void Vehicle2::ApplyAttributes()
{
    auto* vehicle = node_->GetOrCreateComponent<RaycastVehicle>();
    if (emittersCreated)
        return;
    for (const auto& connectionPoint : connectionPoints_)
    {
        CreateEmitter(connectionPoint);
    }
    emittersCreated = true;
}

void Vehicle2::FixedUpdate(float timeStep)
{
    float brakingForce = 0.0f;
    auto* vehicle = node_->GetComponent<RaycastVehicle>();
    // Read controls
    const auto vel = GetVelocity();
    const float newSteering = vel.x_;
    float accelerator = vel.z_;
    if (accelerator < 0.0f)
        accelerator *= 0.5f;

    if (inputMap_->Evaluate("Brake"))
    {
        brakingForce = 1.0f;
    }
    // When steering, wake up the wheel rigidbodies so that their orientation is updated
    if (newSteering != 0.0f)
    {
        SetSteering(GetSteering() * 0.95f + newSteering * 0.05f);
    }
    else
    {
        SetSteering(GetSteering() * 0.8f + newSteering * 0.2f);
    }
    // apply forces

    vehicle->UpdateInput(steering_, accelerator, brakingForce);
}

void Vehicle2::PostUpdate(float timeStep)
{
    auto* vehicle = node_->GetComponent<RaycastVehicle>();
    auto* vehicleBody = node_->GetComponent<RigidBody>();
    Vector3 velocity = vehicleBody->GetLinearVelocity();
    Vector3 accel = (velocity - prevVelocity_) / timeStep;
    float planeAccel = Vector3(accel.x_, 0.0f, accel.z_).Length();
    for (int i = 0; i < vehicle->GetNumWheels(); i++)
    {
        Node* emitter = particleEmitterNodeList_[i];
        auto* wheel = vehicle->GetWheel(i);
        auto* particleEmitter = emitter->GetComponent<ParticleEmitter>();
        if (wheel->IsInContact()
            && (wheel->GetSkidInfoCumulative() < 0.9f || wheel->GetBrakeValue() > 2.0f ||
            planeAccel > 15.0f))
        {
            particleEmitterNodeList_[i]->SetWorldPosition(wheel->GetContactPosition());
            if (!particleEmitter->IsEmitting())
            {
                particleEmitter->SetEmitting(true);
            }
            URHO3D_LOGDEBUG("GetWheelSkidInfoCumulative() = " + ea::to_string(wheel->GetSkidInfoCumulative()) + " "
                + ea::to_string(vehicle->GetMaxSideSlipSpeed()));
            /* TODO: Add skid marks here */
        }
        else if (particleEmitter->IsEmitting())
        {
            particleEmitter->SetEmitting(false);
        }
    }
    prevVelocity_ = velocity;
}

void Vehicle2::SetPitch(float pitch)
{
    MoveAndOrbitComponent::SetPitch(Clamp(pitch, 0.0f, 80.0f));
}
