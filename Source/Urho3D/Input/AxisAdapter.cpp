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

void AxisAdapter::SetMinValue(float value)
{
    minValue_ = value;
}

void AxisAdapter::SetMaxValue(float value)
{
    maxValue_ = value;
}

void AxisAdapter::SetSensitivity(float value)
{
    sensitivity_ = value;
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
    SerializeOptionalValue(archive, "sensitivity", sensitivity_, 1.0f);
    SerializeOptionalValue(archive, "min", minValue_, -1.0f);
    SerializeOptionalValue(archive, "max", maxValue_, 1.0f);
}

float AxisAdapter::Transform(float value) const
{
    const float result = TransformImpl(value);

    if (!inverted_)
        return result;

    if (value > neutral_)
    {
        return neutral_ - (value - neutral_);
    }
    return neutral_ + (neutral_ - value);
}

/// Transform axis value expect inversion.
float AxisAdapter::TransformImpl(float value) const
{
    // Ensure that max value is always bigger then min value.
    float max = maxValue_;
    float min = minValue_;
    if (max < min)
        ea::swap(max, min);

    // Apply dead zone
    if (value >= neutral_ - deadZone_ && value <= neutral_ + deadZone_)
        return neutral_;

    // Clamp the result.
    if (value >= max)
        return max;
    if (value <= min)
        return min;

    if (value > neutral_)
    {
        const float rangeMin = neutral_ + deadZone_;
        const float srcRange = max - rangeMin;
        const float dstRange = max - neutral_;
        const float normalized = (value - rangeMin) / srcRange;
        const float transformed = Pow(normalized, sensitivity_);
        return neutral_ + transformed * dstRange;
    }
    {
        const float rangeMax = neutral_ - deadZone_;
        const float srcRange = rangeMax - min;
        const float dstRange = neutral_ - min;
        const float normalized = (rangeMax - value) / srcRange;
        const float transformed = Pow(normalized, sensitivity_);
        return neutral_ - transformed * dstRange;
    }
}


} // namespace Urho3D
