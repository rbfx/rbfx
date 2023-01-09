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

#include "../IO/Archive.h"

namespace Urho3D
{

/// Helper class to transform axis value.
class URHO3D_API AxisAdapter
{
public:
    static constexpr float DefaultDeadZone = 0.1f;
    static constexpr float Epsilon = 1.0f/32767.f;

    /// Set dead zone half-width to mitigate axis drift.
    void SetDeadZone(float deadZone);
    /// Get dead zone half-width.
    float GetDeadZone() const { return deadZone_; }

    /// Set both sensitivity values. 0.0 represent linear input mapping.
    void SetSensitivity(float value);
    /// Set positive sensitivity value. 0.0 represent linear input mapping.
    void SetPositiveSensitivity(float value);
    /// Set negative sensitivity value. 0.0 represent linear input mapping.
    void SetNegativeSensitivity(float value);
    /// Get positive sensitivity value.
    float GetPositiveSensitivity() const { return posSensitivity_; }
    /// Get negative sensitivity value.
    float GetNegativeSensitivity() const { return negSensitivity_; }

    /// Set neutral value. Neutral value is transformed into 0.0f.
    void SetNeutralValue(float value);
    /// Get neutral value. Neutral value is transformed into 0.0f.
    float GetNeutralValue() const { return neutral_; }

    /// Set inverted flag.
    void SetInverted(bool inverted);
    /// Is axis inverted.
    bool IsInverted() const { return inverted_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    /// Transform axis value. The output is normalized around neutral position into range -1..1.
    float Transform(float value) const;

private:
    /// Convert sensitivity to exponent.
    float GetExponent(float sensitivity) const;
    /// Joystick dead zone half-width.
    float deadZone_{DefaultDeadZone};
    /// Positive sensitivity value.
    float posSensitivity_{0.0f};
    /// Negative sensitivity value.
    float negSensitivity_{0.0f};
    /// Neutral value.
    float neutral_{0.0f};
    /// Is axis inverted.
    bool inverted_{false};
};

} // namespace Urho3D
