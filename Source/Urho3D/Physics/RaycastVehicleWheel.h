//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "../Scene/Component.h"

namespace Urho3D
{
struct RaycastVehicleData;

class RaycastVehicle;

class URHO3D_API RaycastVehicleWheel : public Component
{
    URHO3D_OBJECT(RaycastVehicleWheel, Component);

public:
    static constexpr float DefaultWheelRadius = 1.0f;
    static const Vector3 DefaultWheelDirection;
    static const Vector3 DefaultWheelAxle;
    static constexpr float DefaultSuspensionRestLength = 0.2f;
    static constexpr float DefaultMaxSuspensionTravel = 0.5f;
    static constexpr float DefaultSuspensionStiffness = 5.88f;
    static constexpr float DefaultSuspensionCompression = 0.83f;
    static constexpr float DefaultSuspensionDamping = 0.88f;
    static constexpr float DefaultFrictionSlip = 10.5f;
    static constexpr float DefaultMaxSuspensionForce = 6000.f;
    static constexpr float DefaultSkidInfoCumulative = 1.0f;
    static constexpr float DefaultSideSlipSpeed = 0.0f;
    static constexpr float DefaultRollInfluence = 0.1f;
    static constexpr float DefaultSteeringFactor = 0.0f;
    static constexpr float DefaultBrakeFactor = 1.0f;
    static constexpr float DefaultEngineFactor = 1.0f;
    static constexpr float DefaultSlidingFactor = 1.0f;

    /// Construct.
    explicit RaycastVehicleWheel(Urho3D::Context* context);
    /// Destruct.
    ~RaycastVehicleWheel() override;

    /// Register object factory and attributes.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Set wheel index. Executed by vehicle.
    void SetWheelIndex(unsigned index);
    /// Get wheel index in the vehicle. Returns UINT_MAX if not added to a vehicle.
    unsigned GetWheelIndex() const { return wheelIndex_; }

    /// Get wheel connection point in vehicle space.
    Vector3 GetConnectionPoint() const;
    /// Set wheel connection point in vehicle space.
    void SetConnectionPoint(Vector3 point);

    /// Get wheel node position relative to connection point.
    Vector3 GetOffset() const;
    /// Set wheel node position relative to connection point.
    void SetOffset(Vector3 point);

    /// Get wheel node rotation relative to connection point.
    Quaternion GetRotation() const;
    /// Set wheel node rotation relative to connection point.
    void SetRotation(Quaternion point);

    /// Get wheel direction vector.
    Vector3 GetDirection() const;
    /// Set wheel direction vector.
    void SetDirection(Vector3 direction);

    /// Get wheel axle vector.
    Vector3 GetAxle() const;
    /// Set wheel axle vector.
    void SetAxle(Vector3 direction);

    /// Get steering value of particular wheel.
    float GetSteeringValue() const;
    /// Set steering value of particular wheel.
    void SetSteeringValue(float steeringValue);

    /// Get brake value of particular wheel.
    float GetBrakeValue() const;
    /// Set brake value of particular wheel.
    void SetBrakeValue(float value);

    /// Get engine force.
    float GetEngineForce() const;
    /// Set engine force.
    void SetEngineForce(float value);

    /// Get roll influence.
    float GetRollInfluence() const;
    /// Set roll influence.
    void SetRollInfluence(float value);

    /// Get wheel radius.
    float GetRadius() const;
    /// Set wheel radius.
    void SetRadius(float radius);

    /// Get suspension rest length.
    float GetSuspensionRestLength() const;
    /// Set suspension rest length.
    void SetSuspensionRestLength(float length);

    /// Get suspension max travel.
    float GetMaxSuspensionTravel() const;
    /// Set suspension max travel.
    void SetMaxSuspensionTravel(float length);

    /// Get suspension stiffness.
    float GetSuspensionStiffness() const;
    /// Set suspension max travel.
    void SetSuspensionStiffness(float stiffness);

    /// Get wheel damping compression.
    float GetDampingCompression() const;
    /// Set wheel damping compression.
    void SetDampingCompression(float compression);

    /// Get wheel damping relaxation.
    float GetDampingRelaxation() const;
    /// Set wheel damping relaxation.
    void SetDampingRelaxation(float relaxation);

    /// Get friction slip.
    float GetFrictionSlip() const;
    /// Set friction slip.
    void SetFrictionSlip(float slip);

    /// Get max suspension force.
    float GetMaxSuspensionForce() const;
    /// Set max suspension force.
    void SetMaxSuspensionForce(float force);

    /// Get steering factor.
    float GetSteeringFactor() const;
    /// Set steering factor. Steering value is multiplied by the factor.
    void SetSteeringFactor(float factor);

    /// Get engine factor.
    float GetEngineFactor() const;
    /// Set engine factor. Engine power is multiplied by the factor.
    void SetEngineFactor(float factor);

    /// Get brake factor.
    float GetBrakeFactor() const;
    /// Set brake factor. Brake power is multiplied by the factor.
    void SetBrakeFactor(float factor);

    /// Get skid info cumulative.
    float GetSlidingFactor() const;
    /// Set sliding factor 0 <= x <= 1. The less the value, more sliding.
    void SetSlidingFactor(float factor);

    /// Get skid info cumulative.
    float GetSkidInfoCumulative() const;
    /// Set skid info cumulative.
    void SetSkidInfoCumulative(float skidInfo);

    /// Get side slip speed.
    float GetSideSlipSpeed() const;
    /// Set side slip speed.
    void SetSideSlipSpeed(float sideSlipSpeed);

    /// Get contact position.
    Vector3 GetContactPosition() const;
    /// Set contact position.  Executed by physics simulation.
    void SetContactPosition(Vector3 value);

    /// Get contact normal.
    Vector3 GetContactNormal() const;
    /// Set contact normal.  Executed by physics simulation.
    void SetContactNormal(Vector3 value);

    /// Is in contact.
    bool IsInContact() const;
    /// Set in contact. Executed by physics simulation.
    void SetInContact(bool isInContact);

    /// Update connection point and rotation.
    void ConnectionPointFromTransform();

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;

    void UpdateWheelAtVehicle();

private:
    /// Wheel connection point.
    Vector3 connectionPoint_{Vector3::ZERO};

    /// Wheel direction.
    Vector3 direction_{DefaultWheelDirection};

    /// Wheel axle.
    Vector3 axle_{DefaultWheelAxle};

    /// Wheel offset from connection point.
    Vector3 offset_{Vector3::ZERO};

    /// Wheel rotation relative to car's body.
    Quaternion rotation_{Quaternion::IDENTITY};

    /// Wheel steering value.
    float steeringValue_{};

    /// Wheel brake value.
    float brake_{};

    /// Wheel engine force.
    float engineForce_{};

    /// Wheel roll influence.
    float rollInfluence_{DefaultRollInfluence};

    /// Wheel radius.
    float radius_{DefaultWheelRadius};

    float suspensionRestLength_{DefaultSuspensionRestLength};
    float maxSuspensionTravel_{DefaultMaxSuspensionTravel};
    float suspensionStiffness_{DefaultSuspensionStiffness};
    float dampingCompression_{DefaultSuspensionCompression};
    float dampingRelaxation_{DefaultSuspensionDamping};
    float frictionSlip_{DefaultFrictionSlip};
    float maxSuspensionForce_{DefaultMaxSuspensionForce};

    float steeringFactor_{DefaultSteeringFactor};
    float engineFactor_{DefaultEngineFactor};
    float brakeFactor_{DefaultBrakeFactor};

    Vector3 contactPosition_{};
    Vector3 contactNormal_{};
    float slidingFactor_{DefaultSlidingFactor};
    float skidInfoCumulative_{DefaultSkidInfoCumulative};
    float sideSlipSpeed_{DefaultSideSlipSpeed};
    bool isInContact{};

    unsigned wheelIndex_{UINT_MAX};
    WeakPtr<RaycastVehicle> vehicle_{};
};

} // namespace Urho3D
