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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"

#include <Urho3D/Input/AxisAdapter.h>
#include <Urho3D/Math/MathDefs.h>

using namespace Urho3D;

TEST_CASE("Axis adapter linear transform")
{
    AxisAdapter adapter;
    adapter.SetDeadZone(0.5f);
    adapter.SetInverted(false);

    CHECK(Equals(0.0f, adapter.Transform(0.0f)));
    CHECK(Equals(0.0f, adapter.Transform(0.5f)));
    CHECK(Equals(0.0f, adapter.Transform(-0.5f)));
    CHECK(Equals(1.0f, adapter.Transform(1.0f)));
    CHECK(Equals(0.5f, adapter.Transform(0.75f)));
    CHECK(Equals(-0.5f, adapter.Transform(-0.75f)));
    CHECK(Equals(-1.0f, adapter.Transform(-1.0f)));

    //Test values beyond range
    CHECK(Equals(1.0f, adapter.Transform(2.0f)));
    CHECK(Equals(-1.0f, adapter.Transform(-2.0f)));

    adapter.SetDeadZone(0.0f);
    adapter.SetNeutralValue(0.5f);
    CHECK(Equals(0.5f, adapter.Transform(0.75f)));
    CHECK(Equals(-0.5f, adapter.Transform(-0.25f)));
}

TEST_CASE("Axis adapter sensitivity")
{
    AxisAdapter adapter;
    adapter.SetDeadZone(0.0f);
    adapter.SetSensitivity(1.0f);

    CHECK(Equals(0.0f, adapter.Transform(0.0f)));
    CHECK(Equals(1.0f, adapter.Transform(1.0f)));
    CHECK(Equals(-1.0f, adapter.Transform(-1.0f)));
    CHECK(Equals(0.0625f, adapter.Transform(0.25f)));
    CHECK(Equals(0.5625f, adapter.Transform(0.75f)));
    CHECK(Equals(-0.0625f, adapter.Transform(-0.25f)));
    CHECK(Equals(-0.5625f, adapter.Transform(-0.75f)));

    adapter.SetSensitivity(-1.0f);

    CHECK(Equals(0.5f, adapter.Transform(0.25f)));
    CHECK(Equals(0.866025388f, adapter.Transform(0.75f)));
    CHECK(Equals(-0.5f, adapter.Transform(-0.25f)));
    CHECK(Equals(-0.866025388f, adapter.Transform(-0.75f)));
}

TEST_CASE("Axis adapter inverted")
{
    AxisAdapter adapter;
    adapter.SetDeadZone(0.0f);
    adapter.SetInverted(true);

    CHECK(Equals(-1.0f, adapter.Transform(1.0f)));
    CHECK(Equals(1.0f, adapter.Transform(-1.0f)));

    adapter.SetNeutralValue(0.5f);

    CHECK(Equals(-1.0f, adapter.Transform(1.0f)));
    CHECK(Equals(1.0f, adapter.Transform(-1.0f)));
}

TEST_CASE("Pedal axis adapter with neutral 1.0")
{
    AxisAdapter adapter;
    adapter.SetDeadZone(0.0f);
    adapter.SetNeutralValue(1.0f);

    CHECK(Equals(0.0f, adapter.Transform(1.0f)));
    CHECK(Equals(-0.5f, adapter.Transform(0.0f)));
    CHECK(Equals(-1.0f, adapter.Transform(-1.0f)));

    adapter.SetSensitivity(-1.0f);
    CHECK(Equals(0.0f, adapter.Transform(1.0f)));
    CHECK(Equals(-0.70710678f, adapter.Transform(0.0f)));
    CHECK(Equals(-1.0f, adapter.Transform(-1.0f)));
}

TEST_CASE("Pedal axis adapter with neutral -1.0")
{
    AxisAdapter adapter;
    adapter.SetDeadZone(0.0f);
    adapter.SetNeutralValue(-1.0f);

    CHECK(Equals(1.0f, adapter.Transform(1.0f)));
    CHECK(Equals(0.5f, adapter.Transform(0.0f)));
    CHECK(Equals(0.0f, adapter.Transform(-1.0f)));

    adapter.SetSensitivity(-1.0f);
    CHECK(Equals(1.0f, adapter.Transform(1.0f)));
    CHECK(Equals(0.70710678f, adapter.Transform(0.0f)));
    CHECK(Equals(0.0f, adapter.Transform(-1.0f)));
}
