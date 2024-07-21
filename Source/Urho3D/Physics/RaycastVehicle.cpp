// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "Urho3D/Physics/RaycastVehicle.h"

#include "PhysicsEvents.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Physics/PhysicsUtils.h"
#include "Urho3D/Physics/PhysicsWorld.h"
#include "Urho3D/Physics/RigidBody.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/ContainerComponent.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/Scene/SceneEvents.h"

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
    bool isDynamicDirty_{true};
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
            btWheelInfoConstructionInfo ci;
            FillWheelInfoConstructionInfo(wheel.wheel_, ci);
            vehicle_->addWheel(ci);
            wheel.isDynamicDirty_ = true;
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

    void FillWheelInfoConstructionInfo(RaycastVehicleWheel* wheel, btWheelInfoConstructionInfo& ci)
    {
        const Vector3 connectionPoint = wheel->GetConnectionPoint();
        const Vector3 wheelDirection = wheel->GetDirection();
        const Vector3 wheelAxle = wheel->GetAxle();

        ci.m_chassisConnectionCS = ToBtVector3(connectionPoint);
        ci.m_wheelDirectionCS = ToBtVector3(wheelDirection);
        ci.m_wheelAxleCS = ToBtVector3(wheelAxle);
        ci.m_suspensionRestLength = wheel->GetSuspensionRestLength();
        ci.m_wheelRadius = wheel->GetRadius();
        ci.m_suspensionStiffness = wheel->GetSuspensionStiffness();
        ci.m_wheelsDampingCompression = wheel->GetDampingCompression();
        ci.m_wheelsDampingRelaxation = wheel->GetDampingRelaxation();
        ci.m_frictionSlip = wheel->GetFrictionSlip();
        ci.m_maxSuspensionTravel = wheel->GetMaxSuspensionTravel();
        ci.m_maxSuspensionForce = wheel->GetMaxSuspensionForce();
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
            btWheelInfoConstructionInfo ci;
            FillWheelInfoConstructionInfo(wheel, ci);
            vehicle_->addWheel(ci);

        }
    }

    void UpdateWheel(int index)
    {
        RaycastVehicleWheel* wheel = wheels_[index].wheel_;

        if (wheels_[index].isStaticDirty_)
        {
            btWheelInfoConstructionInfo ci;
            FillWheelInfoConstructionInfo(wheel, ci);
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

    void RemoveWheel(unsigned index)
    {
        vehicle_->removeWheel(static_cast<int>(index));
        wheels_[index].wheel_->SetWheelIndex(UINT_MAX);
        wheels_.erase_at(index);
        for (; index < wheels_.size(); ++index)
        {
            wheels_[index].wheel_->SetWheelIndex(index);
        }
    }

    WeakPtr<PhysicsWorld> physWorld_;
    ea::vector<RaycastWheelData> wheels_;
    btVehicleRaycaster* vehicleRayCaster_;
    btRaycastVehicle* vehicle_;
    bool added_;
};

RaycastVehicle::RaycastVehicle(Context* context) :
    BaseClassName(context)
{
    RegisterAs<RaycastVehicle>();
    URHO3D_OBSERVE_MODULES(RaycastVehicleWheel, AddWheel, RemoveWheel);
    SetSubscribeToContainerEnabled(true);

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

    URHO3D_ACCESSOR_ATTRIBUTE(
        "Engine Force", GetEngineForce, SetEngineForce, float, DefaultEngineForce, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Braking Force", GetBrakingForce, SetBrakingForce, float, DefaultBrakingForce, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Maximum side slip threshold", float, maxSideSlipSpeed_, 4.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("RPM for wheel motors in air (0=calculate)", float, inAirRPM_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Coordinate system", IntVector3, coordinateSystem_, RIGHT_UP_FORWARD, AM_DEFAULT);
}

void RaycastVehicle::DrawWheelDebugGeometry(unsigned wheelIndex, DebugRenderer* debug, bool depthTest) const
{
    auto* wheel = GetWheel(wheelIndex);

    if (!wheel || !debug || !vehicleData_ || !vehicleData_->vehicle_)
        return;

    vehicleData_->UpdateWheel(wheelIndex);
    auto& wheelInfo = vehicleData_->vehicle_->getWheelInfo(wheelIndex);

    Color wheelColor(0, 1, 1);
    if (wheel->IsInContact())
    {
        wheelColor = Color(0, 0, 1);
    }
    else
    {
        wheelColor = Color(1, 0, 1);
    }

    auto rightAxis = vehicleData_->vehicle_->getRightAxis();
    Vector3 axle = Vector3(wheelInfo.m_worldTransform.getBasis()[0][rightAxis],
        wheelInfo.m_worldTransform.getBasis()[1][rightAxis], wheelInfo.m_worldTransform.getBasis()[2][rightAxis]);

    axle.Normalize();

    const auto wheelPosWS = GetWheelPosition(wheelIndex);

    debug->AddCircle(wheelPosWS, axle, wheelInfo.m_wheelsRadius, wheelColor, 64, depthTest);
    debug->AddLine(wheelPosWS, wheelPosWS + axle, wheelColor, depthTest);
    if (wheel->IsInContact())
    {
        debug->AddCircle(wheel->GetContactPosition(), wheel->GetContactNormal(),
            Urho3D::Max(0.01f, wheelInfo.m_wheelsRadius * 0.2f), wheelColor, 64, depthTest);
    }
}

void RaycastVehicle::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
        node_->AddListener(this);

    OnSetEnabled();
}

void RaycastVehicle::OnMarkedDirty(Node* node)
{
    // If node transform changes in editor update wheels.
    if (!hasSimulated_ && vehicleData_ && vehicleData_->vehicle_)
    {
        const auto physicsWorld = GetScene()->GetComponent<PhysicsWorld>();

        if (physicsWorld && !physicsWorld->IsApplyingTransforms())
        {
            for (int i = 0; i < GetNumWheels(); i++)
            {
                vehicleData_->vehicle_->updateWheelTransform(i, false);
            }
        }
    }
}

void RaycastVehicle::OnEffectiveEnabled(bool enabled)
{
    if (vehicleData_)
        vehicleData_->SetEnabled(enabled);

    Scene* scene = GetScene();
    if (scene)
    {
        Component* world = scene->GetComponent<PhysicsWorld>();
        if (scene)
        {
            if (enabled)
            {
                SubscribeToEvent(world, E_PHYSICSPRESTEP, &RaycastVehicle::FixedUpdate);
                SubscribeToEvent(world, E_PHYSICSPOSTSTEP, &RaycastVehicle::FixedPostUpdate);
                SubscribeToEvent(scene, E_SCENEPOSTUPDATE, &RaycastVehicle::PostUpdate);
            }
            else
            {
                UnsubscribeFromEvent(world, E_PHYSICSPRESTEP);
                UnsubscribeFromEvent(world, E_PHYSICSPOSTSTEP);
                UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
            }
        }
    }
}

void RaycastVehicle::ApplyAttributes()
{
    int index = 0;
    hullBody_ = node_->GetOrCreateComponent<RigidBody>();
    Scene* scene = GetScene();
    vehicleData_->Init(scene, hullBody_, IsEnabledEffective(), coordinateSystem_);
}

void RaycastVehicle::ApplyWheelAttributes(unsigned index)
{
    if (vehicleData_ && index < GetNumWheels())
        vehicleData_->UpdateWheel(index);
}

void RaycastVehicle::AddWheel(RaycastVehicleWheel* wheel)
{
    vehicleData_->AddWheel(wheel);
}

void RaycastVehicle::RemoveWheel(RaycastVehicleWheel* wheel)
{
    if (!wheel)
        return;

    const unsigned index = wheel->GetWheelIndex();
    if (index < vehicleData_->wheels_.size() && vehicleData_->wheels_[index].wheel_ == wheel)
    {
        vehicleData_->RemoveWheel(index);
        wheel->SetWheelIndex(UINT_MAX);
    }
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

void RaycastVehicle::FixedUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsPreStep;
    const float timeStep = eventData[P_TIMESTEP].GetFloat();

    hasSimulated_ = true;
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

void RaycastVehicle::PostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace ScenePostUpdate;
    const float timeStep = eventData[P_TIMESTEP].GetFloat();

    btRaycastVehicle* vehicle = vehicleData_->Get();
    for (int i = 0; i < GetNumWheels(); i++)
    {
        vehicle->updateWheelTransform(i, true);
        btTransform transform = vehicle->getWheelTransformWS(i);
        Vector3 origin = ToVector3(transform.getOrigin());
        Quaternion qRot = ToQuaternion(transform.getRotation());
        const RaycastVehicleWheel* wheel = vehicleData_->wheels_[i].wheel_;
        Node* pWheel = wheel->GetNode();
        auto worldRot = node_->GetWorldRotation();
        if (pWheel)
        {
            pWheel->SetWorldPosition(origin + worldRot * wheel->GetOffset());
            pWheel->SetWorldRotation(qRot * wheel->GetRotation());
        }
    }
}

void RaycastVehicle::FixedPostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsPostUpdate;
    const float timeStep = eventData[P_TIMESTEP].GetFloat();

    btRaycastVehicle* vehicle = vehicleData_->Get();
    const Vector3 velocity = hullBody_->GetLinearVelocity();
    for (int i = 0; i < GetNumWheels(); i++)
    {
        RaycastVehicleWheel* wheel = GetWheel(static_cast<unsigned>(i));
        float skidInfoCumulative = wheel->GetSkidInfoCumulative();
        btWheelInfo& whInfo = vehicle->getWheelInfo(i);
        const bool isInContact = whInfo.m_raycastInfo.m_isInContact;
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

        const float wheelSideSlipSpeed = Abs(ToVector3(whInfo.m_raycastInfo.m_wheelAxleWS).DotProduct(velocity));
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

void RaycastVehicle::UpdateInput(float steering, float engineForceFactor, float brakingForceFactor)
{
    for (unsigned wheelIndex=0; wheelIndex<GetNumWheels(); ++wheelIndex)
    {
        auto* wheel = GetWheel(wheelIndex);
        wheel->SetSteeringValue(wheel->GetSteeringFactor() * steering);
        wheel->SetEngineForce(wheel->GetEngineFactor() * engineForceFactor * engineForce_);
        wheel->SetBrakeValue(wheel->GetBrakeFactor() * brakingForceFactor * brakingForce_);
    }
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

Vector3 RaycastVehicle::GetWheelPosition(int wheel) const
{
    btRaycastVehicle* vehicle = vehicleData_->Get();
    btTransform transform = vehicle->getWheelTransformWS(wheel);
    Vector3 origin = ToVector3(transform.getOrigin());
    return origin;
}

Quaternion RaycastVehicle::GetWheelRotation(int wheel) const
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
