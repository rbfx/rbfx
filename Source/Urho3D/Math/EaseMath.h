//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2022 the rbfx project.
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

#include "MathDefs.h"

namespace Urho3D
{

static float BackIn(float time)
{
    const float overshoot = 1.70158f;
    return time * time * ((overshoot + 1) * time - overshoot);
}

static float BackOut(float time)
{
    const float overshoot = 1.70158f;

    time = time - 1;
    return time * time * ((overshoot + 1) * time + overshoot) + 1;
}

static float BackInOut(float time)
{
    const float overshoot = 1.70158f * 1.525f;

    time = time * 2;
    if (time < 1)
        return time * time * ((overshoot + 1) * time - overshoot) / 2;

    time = time - 2;
    return time * time * ((overshoot + 1) * time + overshoot) / 2 + 1;
}

static float BounceOut(float time)
{
    if (time < 1 / 2.75)
        return 7.5625f * time * time;

    if (time < 2 / 2.75)
    {
        time -= 1.5f / 2.75f;
        return 7.5625f * time * time + 0.75f;
    }

    if (time < 2.5 / 2.75)
    {
        time -= 2.25f / 2.75f;
        return 7.5625f * time * time + 0.9375f;
    }

    time -= 2.625f / 2.75f;
    return 7.5625f * time * time + 0.984375f;
}

static float BounceIn(float time) { return 1.0f - BounceOut(1.0f - time); }

static float BounceInOut(float time)
{
    if (time < 0.5f)
    {
        time = time * 2;
        return (1 - BounceOut(1 - time)) * 0.5f;
    }

    return BounceOut(time * 2 - 1) * 0.5f + 0.5f;
}

static float SineOut(float time) { return Urho3D::Sin(time * 90.0f); }

static float SineIn(float time) { return -1.0f * Urho3D::Cos(time * 90.0f) + 1.0f; }

static float SineInOut(float time) { return -0.5f * (Urho3D::Cos(180.0f * time) - 1.0f); }

static float ExponentialOut(float time)
{
    return time == 1.0f ? 1.0f : -Urho3D::Pow<float>(2.0f, -10.0f * time / 1.0f) + 1.0f;
}

static float ExponentialIn(float time)
{
    return time == 0.0f ? 0.0f : Pow(2.0f, 10.0f * (time / 1.0f - 1.0f)) - 1.0f * 0.001f;
}

static float ExponentialInOut(float time)
{
    time /= 0.5f;
    if (time < 1)
        return 0.5f * (float)Pow(2.0f, 10.0f * (time - 1.0f));
    return 0.5f * (-(float)Pow(2.0f, -10.0f * (time - 1.0f)) + 2.0f);
}

static float ElasticIn(float time, float period)
{
    if (time == 0 || time == 1)
        return time;

    auto s = period / 4.0f;
    time = time - 1;
    return -(Urho3D::Pow<float>(2, 10 * time) * Sin((time - s) * 180.0f * 2.0f / period));
}

static float ElasticOut(float time, float period)
{
    if (time == 0 || time == 1)
        return time;

    auto s = period / 4;
    return (Urho3D::Pow<float>(2, -10 * time) * Urho3D::Sin((time - s) * 180.0f * 2.0f / period) + 1);
}

static float ElasticInOut(float time, float period)
{
    if (time == 0 || time == 1)
        return time;

    time = time * 2;
    if (period == 0)
        period = 0.3f * 1.5f;

    auto s = period / 4.0f;

    time = time - 1;
    if (time < 0)
        return (float)(-0.5f * Urho3D::Pow<float>(2, 10 * time) * Urho3D::Sin((time - s) * 360.0f / period));
    return (float)(Urho3D::Pow<float>(2, -10 * time) * Urho3D::Sin((time - s) * 360.0f / period) * 0.5f + 1);
}

} // namespace Urho3D
