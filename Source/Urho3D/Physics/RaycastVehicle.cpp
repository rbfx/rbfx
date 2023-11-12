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

#include "../Precompiled.h"

#include "Urho3D/Physics/RaycastVehicle.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Physics/PhysicsUtils.h"
#include "Urho3D/Physics/PhysicsWorld.h"
#include "Urho3D/Physics/RigidBody.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Graphics/DebugRenderer.h"

#include <Bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <Bullet/BulletDynamics/Vehicle/btRaycastVehicle.h>

namespace Urho3D
{

const IntVector3 RaycastVehicle::RIGHT_UP_FORWARD(0, 1, 2);
const IntVector3 RaycastVehicle::RIGHT_FORWARD_UP(0, 2, 1);
const IntVector3 RaycastVehicle::UP_FORWARD_RIGHT(1, 2, 0);
const IntVector3 RaycastVehicle::UP_RIGHT_FORWARD(1, 0, 2);
const IntVector3 RaycastVehicle::FORWARD_RIGHT_UP(2, 0, 1);
const IntVector3 RaycastVehicle::FORWARD_UP_RIGHT(2, 1, 0);

struct RaycastWheelData
{
    SharedPtr<RaycastVehicleWheel> wheel_;
    bool isStaticDirty_{};
    bool isDynamicDirty_{};
};

struct RaycastVehicleData
{
    RaycastVehicleData()
    {
        vehicleRayCaster_ = nullptr;
        vehicle_ = nullptr;
        added_ = false;
    }

    ~RaycastVehicleData()
    {
        delete vehicleRayCaster_;

        vehicleRayCaster_ = nullptr;
        if (vehicle_)
        {
            if (physWorld_ && added_)
            {
                btDynamicsWorld* pbtDynWorld = physWorld_->GetWorld();
                if (pbtDynWorld)
                    pbtDynWorld->removeAction(vehicle_);
                added_ = false;
            }
            delete vehicle_;
        }
        vehicle_ = nullptr;
    }

    btRaycastVehicle* Get()
    {
        return vehicle_;
    }

    void Init(Scene* scene, RigidBody* body, bool enabled, const IntVector3& coordinateSystem)
    {
        auto* pPhysWorld = scene->GetComponent<PhysicsWorld>();
        btDynamicsWorld* pbtDynWorld = pPhysWorld->GetWorld();
        if (!pbtDynWorld)
            return;

        // Delete old vehicle & action first
        delete vehicleRayCaster_;
        if (vehicle_)
        {
            if (added_)
                pbtDynWorld->removeAction(vehicle_);
            delete vehicle_;
        }

        vehicleRayCaster_ = new btDefaultVehicleRaycaster(pbtDynWorld);
        btRigidBody* bthullBody = body->GetBody();
        vehicle_ = new btRaycastVehicle(bthullBody, vehicleRayCaster_);
        if (enabled)
        {
            pbtDynWorld->addAction(vehicle_);
            added_ = true;
        }

        SetCoordinateSystem(coordinateSystem);
        physWorld_ = pPhysWorld;

        for (auto& wheel: wheels_)
        {
            AddWheelImpl(wheel.wheel_);
        }
    }

    void SetCoordinateSystem(const IntVector3& coordinateSystem)
    {
        if (vehicle_)
            vehicle_->setCoordinateSystem(coordinateSystem.x_, coordinateSystem.y_, coordinateSystem.z_);
    }

    void SetEnabled(bool enabled)
    {
        if (!physWorld_ || !vehicle_)
            return;
        btDynamicsWorld* pbtDynWorld = physWorld_->GetWorld();
        if (!pbtDynWorld)
            return;

        if (enabled && !added_)
        {
            pbtDynWorld->addAction(vehicle_);
            added_ = true;
        }
        else if (!enabled && added_)
        {
            pbtDynWorld->removeAction(vehicle_);
            added_ = false;
        }
    }

    void AddWheelImpl(RaycastVehicleWheel* wheel)
    {
        Vector3 connectionPoint = wheel->GetConnectionPoint();
        Vector3 wheelDirection = wheel->GetDirection();
        Vector3 wheelAxle = wheel->GetAxle();

        btWheelInfoConstructionInfo ci;

        ci.m_chassisConnectionCS = btVector3(connectionPoint.x_, connectionPoint.y_, connectionPoint.z_);
        ci.m_wheelDirectionCS = btVector3(wheelDirection.x_, wheelDirection.y_, wheelDirection.z_);
        ci.m_wheelAxleCS = btVector3(wheelAxle.x_, wheelAxle.y_, wheelAxle.z_);
        ci.m_suspensionRestLength = wheel->GetSuspensionRestLength();
        ci.m_wheelRadius = wheel->GetRadius();
        ci.m_suspensionStiffness = wheel->GetSuspensionStiffness();
        ci.m_wheelsDampingCompression = wheel->GetDampingCompression();
        ci.m_wheelsDampingRelaxation = wheel->GetDampingRelaxation();
        ci.m_frictionSlip = wheel->GetFrictionSlip();
        ci.m_maxSuspensionTravel = wheel->GetMaxSuspensionTravel();
        ci.m_maxSuspensionForce = wheel->GetMaxSuspensionForce();

        vehicle_->addWheel(ci);
    }

    void AddWheel(RaycastVehicleWheel* wheel)
    {
        if (!wheel)
            return;

        RaycastWheelData wheelData;
        unsigned index = wheels_.size();
        wheelData.wheel_ = wheel;
        wheels_.emplace_back(wheelData);
        wheel->SetWheelIndex(index);

        if (vehicle_)
        {
            assert(wheels_.size() == vehicle_->getNumWheels() + 1);
            AddWheelImpl(wheel);
        }
    }

    void UpdateWheel(int index)
    {
        RaycastVehicleWheel* wheel = wheels_[index].wheel_;

        if (wheels_[index].isStaticDirty_)
        {
            btWheelInfoConstructionInfo ci;
            Vector3 connectionPoint = wheel->GetConnectionPoint();
            Vector3 wheelDirection = wheel->GetDirection();
            Vector3 wheelAxle = wheel->GetAxle();

            ci.m_chassisConnectionCS = btVector3(connectionPoint.x_, connectionPoint.y_, connectionPoint.z_);
            ci.m_wheelDirectionCS = btVector3(wheelDirection.x_, wheelDirection.y_, wheelDirection.z_);
            ci.m_wheelAxleCS = btVector3(wheelAxle.x_, wheelAxle.y_, wheelAxle.z_);
            ci.m_suspensionRestLength = wheel->GetSuspensionRestLength();
            ci.m_wheelRadius = wheel->GetRadius();
            ci.m_suspensionStiffness = wheel->GetSuspensionStiffness();
            ci.m_wheelsDampingCompression = wheel->GetDampingCompression();
            ci.m_wheelsDampingRelaxation = wheel->GetDampingRelaxation();
            ci.m_frictionSlip = wheel->GetFrictionSlip();
            ci.m_maxSuspensionTravel = wheel->GetMaxSuspensionTravel();
            ci.m_maxSuspensionForce = wheel->GetMaxSuspensionForce();

            vehicle_->updateWheel(index, ci);
            wheels_[index].isStaticDirty_ = false;
        }

        if (wheels_[index].isDynamicDirty_)
        {
            vehicle_->setSteeringValue(wheel->GetSteeringValue(), index);
            vehicle_->setBrake(wheel->GetBrakeValue(), index);
            vehicle_->applyEngineForce(wheel->GetEngineForce(), index);
            wheels_[index].isDynamicDirty_ = false;
        }
    }

    unsigned RemoveWheel(RaycastVehicleWheel* wheel)
    {
        for (unsigned index=0; index<wheels_.size(); ++index)
        {
            if (wheels_[index].wheel_ == wheel)
            {
                vehicle_->removeWheel(static_cast<int>(index));
                for (; index < wheels_.size(); ++index)
                {
                    wheels_[index].wheel_->SetWheelIndex(index);
                }
                return index;
            }
        }
        return UINT_MAX;
    }

    WeakPtr<PhysicsWorld> physWorld_;
    ea::vector<RaycastWheelData> wheels_;
    btVehicleRaycaster* vehicleRayCaster_;
    btRaycastVehicle* vehicle_;
    bool added_;
};

RaycastVehicle::RaycastVehicle(Context* context) :
    LogicComponent(context)
{
    // fixed update() for inputs and post update() to sync wheels for rendering
    SetUpdateEventMask(USE_FIXEDUPDATE | USE_FIXEDPOSTUPDATE | USE_POSTUPDATE);
    vehicleData_ = new RaycastVehicleData();
    coordinateSystem_ = RIGHT_UP_FORWARD;
    activate_ = false;
    inAirRPM_ = 0.0f;
    maxSideSlipSpeed_ = 4.0f;
}

RaycastVehicle::~RaycastVehicle()
{
    delete vehicleData_;
}

void RaycastVehicle::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RaycastVehicle>(Category_Physics);

    URHO3D_ATTRIBUTE("Maximum side slip threshold", float, maxSideSlipSpeed_, 4.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("RPM for wheel motors in air (0=calculate)", float, inAirRPM_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Coordinate system", IntVector3, coordinateSystem_, RIGHT_UP_FORWARD, AM_DEFAULT);
}

void RaycastVehicle::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && vehicleData_ && vehicleData_->vehicle_)
    {
        for (int v = 0; v < vehicleData_->vehicle_->getNumWheels(); v++)
        {
            auto& wheelInfo = vehicleData_->vehicle_->getWheelInfo(v);
            Color wheelColor(0, 1, 1);
            if (wheelInfo.m_raycastInfo.m_isInContact)
            {
                wheelColor = Color(0, 0, 1);
            }
            else
            {
                wheelColor = Color(1, 0, 1);
            }

            auto rightAxis = vehicleData_->vehicle_->getRightAxis();
            Vector3 axle = Vector3(wheelInfo.m_worldTransform.getBasis()[0][rightAxis],
                wheelInfo.m_worldTransform.getBasis()[1][rightAxis],
                wheelInfo.m_worldTransform.getBasis()[2][rightAxis]);

            axle.Normalize();

            auto wheelPosWS = GetWheelPosition(v);

            debug->AddCircle(wheelPosWS, axle, wheelInfo.m_wheelsRadius, wheelColor, 64, depthTest);
            //debug->AddCircle(wheelPosWS + axle * wheelInfo.wh, axle, wheelInfo.m_wheelsRadius, wheelColor, 64, depthTest);

            debug->AddLine(wheelPosWS, wheelPosWS + axle, wheelColor, false);
            // debug->AddLine(wheelPosWS, ToVector3(wheelInfo.m_raycastInfo.m_contactPointWS), wheelColor, false);
            debug->AddCircle(ToVector3(wheelInfo.m_raycastInfo.m_contactPointWS),
                ToVector3(wheelInfo.m_raycastInfo.m_contactNormalWS), 0.2f, wheelColor, 64, depthTest);
        }
    }
}

void RaycastVehicle::OnSetEnabled()
{
    if (vehicleData_)
        vehicleData_->SetEnabled(IsEnabledEffective());
}

void RaycastVehicle::ApplyAttributes()
{
    int index = 0;
    hullBody_ = node_->GetOrCreateComponent<RigidBody>();
    Scene* scene = GetScene();
    vehicleData_->Init(scene, hullBody_, IsEnabledEffective(), coordinateSystem_);
}

void RaycastVehicle::AddWheel(RaycastVehicleWheel* wheel)
{
    vehicleData_->AddWheel(wheel);
}

void RaycastVehicle::RemoveWheel(RaycastVehicleWheel* wheel)
{
    unsigned index = vehicleData_->RemoveWheel(wheel);
}

RaycastVehicleWheel* RaycastVehicle::GetWheel(unsigned index) const
{
    if (index >= vehicleData_->wheels_.size())
        return nullptr;

    return vehicleData_->wheels_[index].wheel_;
}

void RaycastVehicle::Init()
{
    hullBody_ = node_->GetOrCreateComponent<RigidBody>();
    Scene* scene = GetScene();
    vehicleData_->Init(scene, hullBody_, IsEnabledEffective(), coordinateSystem_);
}

void RaycastVehicle::FixedUpdate(float timeStep)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    for (int i = 0; i < GetNumWheels(); i++)
    {
        vehicleData_->UpdateWheel(i);

        btWheelInfo whInfo = vehicle->getWheelInfo(i);
        if (whInfo.m_engineForce != 0.0f || whInfo.m_steering != 0.0f)
        {
            if (!hullBody_->IsActive())
                hullBody_->Activate();
        }
    }
}

void RaycastVehicle::PostUpdate(float timeStep)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    for (int i = 0; i < GetNumWheels(); i++)
    {
        vehicle->updateWheelTransform(i, true);
        btTransform transform = vehicle->getWheelTransformWS(i);
        Vector3 origin = ToVector3(transform.getOrigin());
        Quaternion qRot = ToQuaternion(transform.getRotation());
        RaycastVehicleWheel* wheel = vehicleData_->wheels_[i].wheel_;
        Node* pWheel = wheel->GetNode();
        auto worldRot = node_->GetWorldRotation();
        if (pWheel)
        {
            pWheel->SetWorldPosition(origin + worldRot * wheel->GetOffset());
            pWheel->SetWorldRotation(qRot * wheel->GetRotation());
        }
    }
}

void RaycastVehicle::FixedPostUpdate(float timeStep)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    Vector3 velocity = hullBody_->GetLinearVelocity();
    for (int i = 0; i < GetNumWheels(); i++)
    {
        RaycastVehicleWheel* wheel = GetWheel(static_cast<unsigned>(i));
        float skidInfoCumulative = wheel->GetSkidInfoCumulative();
        btWheelInfo& whInfo = vehicle->getWheelInfo(i);
        bool isInContact = whInfo.m_raycastInfo.m_isInContact;
        wheel->SetInContact(isInContact);
        if (!isInContact && wheel->GetEngineForce() != 0.0f)
        {
            float delta;
            if (inAirRPM_ != 0.0f)
            {
                delta = inAirRPM_ * timeStep / 60.0f;
            }
            else
            {
                delta = 8.0f * wheel->GetEngineForce() * timeStep / (hullBody_->GetMass() * wheel->GetRadius());
            }
            if (Abs(whInfo.m_deltaRotation) < Abs(delta))
            {
                whInfo.m_rotation += delta - whInfo.m_deltaRotation;
                whInfo.m_deltaRotation = delta;
            }
            if (skidInfoCumulative > 0.05f)
            {
                skidInfoCumulative -= 0.002f;
            }
        }
        else
        {
            wheel->SetContactPosition(ToVector3(whInfo.m_raycastInfo.m_contactPointWS));
            wheel->SetContactNormal(ToVector3(whInfo.m_raycastInfo.m_contactNormalWS));
            skidInfoCumulative = wheel->GetSlidingFactor();
        }

        float wheelSideSlipSpeed = Abs(ToVector3(whInfo.m_raycastInfo.m_wheelAxleWS).DotProduct(velocity));
        if (wheelSideSlipSpeed > maxSideSlipSpeed_)
        {
            skidInfoCumulative = Clamp(skidInfoCumulative, 0.0f, 0.89f);
        }
        wheel->SetSideSlipSpeed(wheelSideSlipSpeed);
        wheel->SetSkidInfoCumulative(skidInfoCumulative);
    }
}

void RaycastVehicle::SetMaxSideSlipSpeed(float speed)
{
    maxSideSlipSpeed_ = speed;
}

float RaycastVehicle::GetMaxSideSlipSpeed() const
{
    return maxSideSlipSpeed_;
}

void RaycastVehicle::ResetSuspension()
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->resetSuspension();
}

void RaycastVehicle::UpdateWheelTransform(int wheel, bool interpolated)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    vehicle->updateWheelTransform(wheel, interpolated);
}

Vector3 RaycastVehicle::GetWheelPosition(int wheel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btTransform transform = vehicle->getWheelTransformWS(wheel);
    Vector3 origin = ToVector3(transform.getOrigin());
    return origin;
}

Quaternion RaycastVehicle::GetWheelRotation(int wheel)
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    const btTransform& transform = vehicle->getWheelTransformWS(wheel);
    Quaternion rotation = ToQuaternion(transform.getRotation());
    return rotation;
}

Vector3 RaycastVehicle::GetWheelConnectionPoint(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btWheelInfo whInfo = vehicle->getWheelInfo(wheel);
    return ToVector3(whInfo.m_chassisConnectionPointCS);
}

int RaycastVehicle::GetNumWheels() const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    return vehicle ? vehicle->getNumWheels() : 0;
}

void RaycastVehicle::SetInAirRPM(float rpm)
{
    inAirRPM_ = rpm;
}

float RaycastVehicle::GetInAirRPM() const
{
    return inAirRPM_;
}

void RaycastVehicle::SetCoordinateSystem(const IntVector3& coordinateSystem)
{
    coordinateSystem_ = coordinateSystem;
    vehicleData_->SetCoordinateSystem(coordinateSystem_);
}

void RaycastVehicle::ResetWheels()
{
    ResetSuspension();
    for (int i = 0; i < GetNumWheels(); i++)
    {
        UpdateWheelTransform(i, true);
        Vector3 origin = GetWheelPosition(i);
        Node* wheelNode = GetWheel(i)->GetNode();
        wheelNode->SetWorldPosition(origin);
    }
}

void RaycastVehicle::InvalidateStaticWheelParameters(unsigned wheelIndex)
{
    if (wheelIndex < vehicleData_->wheels_.size())
    {
        vehicleData_->wheels_[wheelIndex].isStaticDirty_ = true;
    }
}

void RaycastVehicle::InvalidateDynamicWheelParameters(unsigned wheelIndex)
{
    if (wheelIndex < vehicleData_->wheels_.size())
    {
        vehicleData_->wheels_[wheelIndex].isDynamicDirty_ = true;
    }
}

} // namespace Urho3D
