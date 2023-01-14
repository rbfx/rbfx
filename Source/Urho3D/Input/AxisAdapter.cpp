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

#include "../Precompiled.h"

#include "../Input/AxisAdapter.h"
#include "../IO/ArchiveSerializationBasic.h"

namespace Urho3D
{

void AxisAdapter::SetDeadZone(float deadZone)
{
    deadZone_ = Urho3D::Max(deadZone, 0.0f);
}

void AxisAdapter::SetSensitivity(float value)
{
    posSensitivity_ = negSensitivity_ = value;
}

void AxisAdapter::SetPositiveSensitivity(float value)
{
    posSensitivity_ = value;
}

void AxisAdapter::SetNegativeSensitivity(float value)
{
    negSensitivity_ = value;
}

void AxisAdapter::SetNeutralValue(float value)
{
    neutral_ = value;
}

void AxisAdapter::SetInverted(bool inverted)
{
    inverted_ = inverted;
}

void AxisAdapter::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "deadZone", deadZone_, DefaultDeadZone);
    SerializeOptionalValue(archive, "inverted", inverted_, false);
    SerializeOptionalValue(archive, "neutral", neutral_, 0.0f);
    SerializeOptionalValue(archive, "posSensitivity", posSensitivity_, 0.0f);
    SerializeOptionalValue(archive, "negSensitivity", negSensitivity_, 0.0f);
}


float AxisAdapter::Transform(float value) const
{
    if (inverted_)
        value = -value;

    // Apply dead zone. Neutral position is mapped to 0.0.
    if (value >= neutral_ - deadZone_ - Epsilon && value <= neutral_ + deadZone_ + Epsilon)
        return 0.0f;

    // Clamp the result.
    if (value >= 1.0f - Epsilon)
        return 1.0f;
    if (value <= -1.0f + Epsilon)
        return -1.0f;

    if (value > neutral_)
    {
        const float rangeMin = neutral_ + deadZone_;
        const float srcRange = 1.0f - rangeMin;
        const float normalized = (value - rangeMin) / srcRange;
        const float exp = GetExponent(posSensitivity_);
        const float transformed = Pow(normalized, exp);
        return transformed;
    }
    const float rangeMax = neutral_ - deadZone_;
    const float srcRange = rangeMax + 1.0f;
    const float normalized = (rangeMax - value) / srcRange;
    const float exp = GetExponent(negSensitivity_);
    const float transformed = Pow(normalized, exp);
    return -transformed;
}

/// Convert sensitivity to exponent.
float AxisAdapter::GetExponent(float sensitivity) const
{
    if (sensitivity > 0)
        return 1.0f + sensitivity;
    return 1.0f/(1.0f - sensitivity);
}

} // namespace Urho3D
