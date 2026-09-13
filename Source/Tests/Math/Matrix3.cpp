// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <EASTL/variant.h>

using namespace Urho3D;

TEST_CASE("Matrix3 from Axis Angle")
{
    Vector3 axis(1, -2, 3);
    axis.Normalize();
    const Matrix3 matrix(30.0f, axis);
    const Matrix3 expected = Quaternion(30.0f, axis).RotationMatrix();

    CHECK(matrix.Equals(expected));
}
