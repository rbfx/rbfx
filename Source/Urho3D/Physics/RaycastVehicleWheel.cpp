//
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

#include "Urho3D/Precompiled.h"

#include "Urho3D/Physics/RaycastVehicleWheel.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Physics/RaycastVehicle.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

const Vector3 RaycastVehicleWheel::DefaultWheelDirection = Vector3(0.0f, -1.0f, 0.0f);
const Vector3 RaycastVehicleWheel::DefaultWheelAxle = Vector3(1.0f, 0.0f, 0.0f);

RaycastVehicleWheel::RaycastVehicleWheel(Context* context)
    : Component(context)
{
}

RaycastVehicleWheel::~RaycastVehicleWheel()
{
}

void RaycastVehicleWheel::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RaycastVehicleWheel>(Category_Physics);

    URHO3D_ACCESSOR_ATTRIBUTE(
        "Connection Point", GetConnectionPoint, SetConnectionPoint, Vector3, DefaultWheelDirection, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Offset", GetOffset, SetOffset, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion::IDENTITY, AM_DEFAULT);
    URHO3D_ACTION_STATIC_LABEL("Set Connection", ConnectionPointFromTransform, "Update connection point and rotation");
    URHO3D_ACCESSOR_ATTRIBUTE("Direction", GetDirection, SetDirection, Vector3, Vector3::DOWN, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Axle", GetAxle, SetAxle, Vector3, DefaultWheelAxle, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Roll Influence", GetRollInfluence, SetRollInfluence, float, DefaultRollInfluence, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, DefaultWheelRadius, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Steering Factor", GetSteeringFactor, SetSteeringFactor, float, DefaultSteeringFactor, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Engine Factor", GetEngineFactor, SetEngineFactor, float, DefaultEngineFactor, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Brake Factor", GetBrakeFactor, SetBrakeFactor, float, DefaultBrakeFactor, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Suspension Rest Length", GetSuspensionRestLength, SetSuspensionRestLength, float,
        DefaultSuspensionRestLength, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Suspension Travel", GetMaxSuspensionTravel, SetMaxSuspensionTravel, float,
        DefaultMaxSuspensionTravel, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Suspension Stiffness", GetSuspensionStiffness, SetSuspensionStiffness, float,
        DefaultSuspensionStiffness, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Damping Compression", GetDampingCompression, SetDampingCompression, float,
        DefaultSuspensionCompression, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Damping Relaxation", GetDampingRelaxation, SetDampingRelaxation, float, DefaultSuspensionDamping, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Friction Slip", GetFrictionSlip, SetFrictionSlip, float, DefaultFrictionSlip, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Suspension Force", GetMaxSuspensionForce, SetMaxSuspensionForce, float,
        DefaultMaxSuspensionForce, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Sliding Factor", GetSlidingFactor, SetSlidingFactor, float, DefaultSlidingFactor, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Skid Info", GetSkidInfoCumulative, SetSkidInfoCumulative, float, DefaultSkidInfoCumulative, AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Steering", GetSteeringValue, SetSteeringValue, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Brake", GetBrakeValue, SetBrakeValue, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Engine Force", GetEngineForce, SetEngineForce, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Side Slip Speed", GetSideSlipSpeed, SetSideSlipSpeed, float, DefaultSideSlipSpeed, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is In Contact", IsInContact, SetInContact, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE(
        "Contact Position", GetContactPosition, SetContactPosition, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Contact Normal", GetContactNormal, SetContactNormal, Vector3, Vector3::ZERO, AM_DEFAULT);
}


void RaycastVehicleWheel::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    depthTest = false;

    if (!vehicle_)
        return;

    vehicle_->DrawWheelDebugGeometry(wheelIndex_, debug, depthTest);
}

void RaycastVehicleWheel::SetWheelIndex(unsigned index)
{
    wheelIndex_ = index;
}

void RaycastVehicleWheel::OnNodeSet(Node* previousNode, Node* currentNode)
{
    UpdateWheelAtVehicle();
    Component::OnNodeSet(previousNode, currentNode);
}

void RaycastVehicleWheel::ApplyAttributes()
{
    UpdateWheelAtVehicle();
    if (vehicle_)
    {
        vehicle_->ApplyWheelAttributes(wheelIndex_);
    }
    Component::ApplyAttributes();
}

void RaycastVehicleWheel::UpdateWheelAtVehicle()
{
    RaycastVehicle* vehicle = nullptr;
    if (node_)
        vehicle = node_->FindComponent<RaycastVehicle>(ComponentSearchFlag::ParentRecursive);

    if (vehicle != vehicle_)
    {
        if (vehicle_)
        {
            vehicle_->RemoveWheel(this);
        }
        vehicle_ = vehicle;
        if (vehicle_)
        {
            vehicle_->AddWheel(this);
        }
    }
}

void RaycastVehicleWheel::ConnectionPointFromTransform()
{
    if (!node_  || !vehicle_)
        return;

    const auto* vehicleNode = vehicle_->GetNode();
    if (!vehicleNode)
        return;

    const auto* carBody = vehicleNode->GetComponent<RigidBody>();
    if (!carBody)
        return;

    const auto centerOfMassTransform = Matrix3x4::FromTranslation(vehicleNode->GetWorldTransform()*carBody->GetCenterOfMass())
        * Matrix3x4::FromRotation(vehicleNode->GetWorldRotation());
    const auto worldToCarBody = centerOfMassTransform.Inverse();
    const auto wheelToVehicle = worldToCarBody * node_->GetWorldTransform();

    // Default wheel position is calculated as following
    // wheel.m_raycastInfo.m_hardPointWS + wheel.m_raycastInfo.m_wheelDirectionWS * wheel.m_raycastInfo.m_suspensionLength
    // where m_hardPointWS is connection point.

    const Vector3 worldSpaceOffset = node_->GetWorldTransform() * offset_;
    const Vector3 worldSpaceConnection = worldSpaceOffset - direction_ * suspensionRestLength_;
    SetConnectionPoint(worldToCarBody * worldSpaceConnection);
    SetRotation((wheelToVehicle).Rotation());
    UpdateWheelAtVehicle();
}

#define DEFINE_ACCESSOR(getter, setter, field, type) \
    type RaycastVehicleWheel::getter() const \
    { \
        return field; \
    } \
    void RaycastVehicleWheel::setter(type value) \
    { \
        if (field != value) \
        { \
            field = value; \
            if (vehicle_) \
                vehicle_->InvalidateStaticWheelParameters(wheelIndex_); \
        } \
    }

/// Define getters and setters for static wheel properties defined by user.
/// These properties usually don't change during the game.

DEFINE_ACCESSOR(GetConnectionPoint, SetConnectionPoint, connectionPoint_, Vector3);
DEFINE_ACCESSOR(GetOffset, SetOffset, offset_, Vector3);
DEFINE_ACCESSOR(GetRotation, SetRotation, rotation_, Quaternion);
DEFINE_ACCESSOR(GetDirection, SetDirection, direction_, Vector3);
DEFINE_ACCESSOR(GetAxle, SetAxle, axle_, Vector3);
DEFINE_ACCESSOR(GetRollInfluence, SetRollInfluence, rollInfluence_, float);
DEFINE_ACCESSOR(GetRadius, SetRadius, radius_, float);
DEFINE_ACCESSOR(GetSuspensionRestLength, SetSuspensionRestLength, suspensionRestLength_, float);
DEFINE_ACCESSOR(GetMaxSuspensionTravel, SetMaxSuspensionTravel, maxSuspensionTravel_, float);
DEFINE_ACCESSOR(GetSuspensionStiffness, SetSuspensionStiffness, suspensionStiffness_, float);
DEFINE_ACCESSOR(GetDampingCompression, SetDampingCompression, dampingCompression_, float);
DEFINE_ACCESSOR(GetDampingRelaxation, SetDampingRelaxation, dampingRelaxation_, float);
DEFINE_ACCESSOR(GetFrictionSlip, SetFrictionSlip, frictionSlip_, float);
DEFINE_ACCESSOR(GetSlidingFactor, SetSlidingFactor, slidingFactor_, float);
DEFINE_ACCESSOR(GetMaxSuspensionForce, SetMaxSuspensionForce, maxSuspensionForce_, float);
DEFINE_ACCESSOR(GetSteeringFactor, SetSteeringFactor, steeringFactor_, float);
DEFINE_ACCESSOR(GetEngineFactor, SetEngineFactor, engineFactor_, float);
DEFINE_ACCESSOR(GetBrakeFactor, SetBrakeFactor, brakeFactor_, float);

#undef DEFINE_ACCESSOR

/// Define getters and setters for wheel properties that may update frequently with user input.

#define DEFINE_ACCESSOR(getter, setter, field, type) \
    type RaycastVehicleWheel::getter() const \
    { \
        return field; \
    } \
    void RaycastVehicleWheel::setter(type value) \
    { \
        if (field != value) \
        { \
            field = value; \
            if (vehicle_) \
                vehicle_->InvalidateDynamicWheelParameters(wheelIndex_); \
        } \
    }

DEFINE_ACCESSOR(GetSteeringValue, SetSteeringValue, steeringValue_, float);
DEFINE_ACCESSOR(GetBrakeValue, SetBrakeValue, brake_, float);
DEFINE_ACCESSOR(GetEngineForce, SetEngineForce, engineForce_, float);

#undef DEFINE_ACCESSOR

/// Define getters and setters for wheel properties that may update frequently by physics.

#define DEFINE_ACCESSOR(getter, setter, field, type) \
    type RaycastVehicleWheel::getter() const \
    { \
        return field; \
    } \
    void RaycastVehicleWheel::setter(type value) \
    { \
        field = value; \
    }

DEFINE_ACCESSOR(GetSkidInfoCumulative, SetSkidInfoCumulative, skidInfoCumulative_, float);
DEFINE_ACCESSOR(GetSideSlipSpeed, SetSideSlipSpeed, sideSlipSpeed_, float);
DEFINE_ACCESSOR(IsInContact, SetInContact, isInContact, bool);
DEFINE_ACCESSOR(GetContactPosition, SetContactPosition, contactPosition_, Vector3);
DEFINE_ACCESSOR(GetContactNormal, SetContactNormal, contactNormal_, Vector3);

#undef DEFINE_ACCESSOR

} // namespace Urho3D
