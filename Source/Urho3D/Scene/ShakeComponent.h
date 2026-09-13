// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/PerlinNoise.h"
#include "Urho3D/Scene/LogicComponent.h"

namespace Urho3D
{

/// Component that randomly displaces the node around (0,0,0) coordinate.
/// Perfect for camera shake effect.
///
/// Math for Game Programmers: Juicing Your Cameras With Math by Squirrel Eiserloh
/// https://www.gdcvault.com/play/1033548/Math-for-Game-Programmers-Juicing
///
class URHO3D_API ShakeComponent : public LogicComponent
{
    URHO3D_OBJECT(ShakeComponent, LogicComponent)

public:
    inline static constexpr float DEFAULT_TIMESCALE{16.0f};

    explicit ShakeComponent(Context* context);
    ~ShakeComponent() override;

    static void RegisterObject(Context* context);

    
    /// Set time scale. This is a multiplier for Perlin Noise argument.
    void SetTimeScale(float value);
    /// Get time scale.
    float GetTimeScale() const { return timeScale_; }

    /// Increase trauma value.
    void AddTrauma(float value);
    /// Set trauma value.
    void SetTrauma(float value);
    /// Get trauma value.
    float GetTrauma() const { return trauma_; }

    /// Set trauma power value.
    void SetTraumaPower(float value);
    /// Get trauma power value.
    float GetTraumaPower() const { return traumaPower_; }

    /// Set trauma falloff (how many units to loose per second).
    void SetTraumaFalloff(float value);
    /// Get trauma falloff (how many units to loose per second).
    float GetTraumaFalloff() const { return traumaPower_; }

    /// Set shift range.
    void SetShiftRange(const Vector3& value);
    /// Get shift range.
    const Vector3& GetShiftRange() const { return shiftRange_; }

    /// Set rotation range.
    void SetRotationRange(const Vector3& value);
    /// Get rotation range.
    const Vector3& GetRotationRange() const { return rotationRange_; }

protected:
    /// Update node position.
    void Update(float timeStep) override;

private:
    /// Perlin noise generator
    PerlinNoise perlinNoise_;

    /// Current time value.
    float time_{};
    /// Current trauma value.
    float trauma_{};
    /// Current trauma power.
    float traumaPower_{2.0f};
    /// Current trauma falloff.
    float traumaFalloff_{1.0f};
    /// Time scale.
    float timeScale_{DEFAULT_TIMESCALE};

    /// Shift range.
    Vector3 shiftRange_{Vector3::ZERO};
    /// Rotation range (Pitch, Yaw, Roll).
    Vector3 rotationRange_{Vector3::ZERO};

    /// Flag indicating that original position and rotation are captured.
    bool hasOriginalPosition_{};
    /// Original position of the node.
    Vector3 originalPosition_{Vector3::ZERO};
    /// Original rotation of the node.
    Quaternion originalRotation_{Quaternion::IDENTITY};
    /// Last known position of the node.
    Vector3 lastKnownPosition_{Vector3::ZERO};
    /// Last known rotation of the node.
    Quaternion lastKnownRotation_{Quaternion::IDENTITY};
};

} // namespace Urho3D
