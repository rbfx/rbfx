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
};

} // namespace Urho3D
