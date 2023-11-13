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

#pragma once

#include "Urho3D/Scene/LogicComponent.h"
#include "Urho3D/Physics/PhysicsUtils.h"
#include "Urho3D/Physics/RigidBody.h"
#include "Urho3D/Physics/RaycastVehicleWheel.h"

namespace Urho3D
{
struct RaycastVehicleData;

class URHO3D_API RaycastVehicle : public LogicComponent
{
    URHO3D_OBJECT(RaycastVehicle, LogicComponent);

public:
    constexpr static float DefaultBrakingForce = 50.0f;
    constexpr static float DefaultEngineForce = 2500.0f;

    /// Construct.
    explicit RaycastVehicle(Urho3D::Context* context);
    /// Destruct.
    ~RaycastVehicle() override;

    /// Register object factory and attributes.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Visualize wheel as debug geometry.
    void DrawWheelDebugGeometry(unsigned index, DebugRenderer* debug, bool depthTest) const;

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Perform post-load after deserialization. Acquire the components from the scene nodes.
    void ApplyAttributes() override;
    /// Immediately apply wheel attributes to physics.
    void ApplyWheelAttributes(unsigned index);

    /// Add a wheel.
    void AddWheel(RaycastVehicleWheel* wheel);
    /// Remove a wheel.
    void RemoveWheel(RaycastVehicleWheel* wheel);
    /// Get wheel.
    RaycastVehicleWheel* GetWheel(unsigned index) const;

    /// Get maximum linear momentum supplied by engine to RigidBody.
    float GetEngineForce() const { return engineForce_; }
    /// Set maximum linear momentum supplied by engine to RigidBody.
    void SetEngineForce(float engineForce) { engineForce_ = engineForce; }

    /// Get rotational momentum preventing (dampening) wheels rotation.
    float GetBrakingForce() const { return brakingForce_; }
    /// Set rotational momentum preventing (dampening) wheels rotation.
    void SetBrakingForce(float brakingForce) { brakingForce_ = brakingForce; }

    /// Update input values for wheels.
    void UpdateInput(float steering, float engineForceFactor, float brakingForceFactor);

    /// Reset all suspension.
    void ResetSuspension();
    /// Sets node initial positions.
    void ResetWheels();

    /// Mark wheel static data as dirty and update it before simulation.
    void InvalidateStaticWheelParameters(unsigned wheelIndex);
    /// Mark wheel dynamic data as dirty and update it before simulation.
    void InvalidateDynamicWheelParameters(unsigned wheelIndex);
    

    /// Update transform for particular wheel.
    void UpdateWheelTransform(int wheel, bool interpolated);

    /// Set side speed which is considered sliding.
    /// @property
    void SetMaxSideSlipSpeed(float speed);
    /// Set revolution per minute value for when wheel doesn't touch ground. If set to 0 (or not set), calculated from
    /// engine force (probably not what you want).
    /// @property
    void SetInAirRPM(float rpm);
    /// Set the coordinate system. The default is (0, 1, 2).
    /// @property
    void SetCoordinateSystem(const IntVector3& coordinateSystem = RIGHT_FORWARD_UP);
    /// Init the vehicle component after creation.
    void Init();
    /// Perform fixed step pre-update.
    void FixedUpdate(float timeStep) override;
    /// Perform fixed step post-update.
    void FixedPostUpdate(float timeStep) override;
    /// Perform variable step post-update.
    void PostUpdate(float timeStep) override;

    /// Get wheel position relative to RigidBody.
    Vector3 GetWheelPosition(int wheel) const;
    /// Get wheel rotation relative to RigidBody.
    Quaternion GetWheelRotation(int wheel) const;
    /// Get wheel connection point relative to RigidBody.
    Vector3 GetWheelConnectionPoint(int wheel) const;
    /// Get number of attached wheels.
    /// @property
    int GetNumWheels() const;

    /// Get side speed which is considered sliding.
    /// @property
    float GetMaxSideSlipSpeed() const;
    /// Get revolution per minute value for when wheel doesn't touch ground.
    /// @property
    float GetInAirRPM() const;
    /// Get the coordinate system.
    /// @property
    IntVector3 GetCoordinateSystem() const { return coordinateSystem_; }

    /// (0, 1, 2) coordinate system (default).
    static const IntVector3 RIGHT_UP_FORWARD;
    /// (0, 2, 1) coordinate system.
    static const IntVector3 RIGHT_FORWARD_UP;
    /// (1, 2, 0) coordinate system.
    static const IntVector3 UP_FORWARD_RIGHT;
    /// (1, 0, 2) coordinate system.
    static const IntVector3 UP_RIGHT_FORWARD;
    /// (2, 0, 1) coordinate system.
    static const IntVector3 FORWARD_RIGHT_UP;
    /// (2, 1, 0) coordinate system.
    static const IntVector3 FORWARD_UP_RIGHT;

private:
    /// If the RigidBody should be activated.
    bool activate_;
    /// Hull RigidBody.
    WeakPtr<RigidBody> hullBody_;
    /// Opaque Bullet data hidden from public.
    RaycastVehicleData* vehicleData_;
    /// Coordinate system.
    IntVector3 coordinateSystem_;
    /// Nodes of all wheels.
    ea::vector<RaycastVehicleWheel*> wheelComponents_;
    /// Revolutions per minute value for in-air motor wheels. FIXME: set this one per wheel.
    float inAirRPM_;
    /// Side slip speed threshold.
    float maxSideSlipSpeed_;
    /// Rotational momentum preventing (dampening) wheels rotation
    float brakingForce_{DefaultBrakingForce};
    /// Maximum linear momentum supplied by engine to RigidBody
    float engineForce_{DefaultEngineForce};
};

}
