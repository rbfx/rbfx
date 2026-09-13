// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
