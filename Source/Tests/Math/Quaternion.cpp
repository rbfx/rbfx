// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <EASTL/variant.h>

using namespace Urho3D;

TEST_CASE("Quaternion from Euler")
{
    const Vector3 angles(10, -20, 30);
    const Matrix3 matrix = Matrix3(angles.y_, Vector3(0, 1, 0)) * Matrix3(angles.x_, Vector3(1, 0, 0))
        * Matrix3(angles.z_, Vector3(0, 0, 1));
    const Matrix3 expected = Quaternion(angles).RotationMatrix();

    CHECK(matrix.Equals(expected));
}

TEST_CASE("Quaternion from Gimbal lock position")
{
    {
        const Vector3 expected(90, -10, 0);
        const Matrix3 expectedMatrix = Matrix3(expected.y_, Vector3(0, 1, 0)) * Matrix3(expected.x_, Vector3(1, 0, 0));
        const Quaternion actualQuaternion = Quaternion(expected);
        const Vector3 actualAngles = actualQuaternion.EulerAngles();
        const Matrix3 actualMatrix = actualQuaternion.RotationMatrix();

        CHECK(expected.Equals(actualAngles));
        CHECK(expectedMatrix.Equals(actualMatrix));
    }
    {
        const Vector3 expected(-90, -10, 0);
        const Matrix3 expectedMatrix = Matrix3(expected.y_, Vector3(0, 1, 0)) * Matrix3(expected.x_, Vector3(1, 0, 0));
        const Quaternion actualQuaternion = Quaternion(expected);
        const Vector3 actualAngles = actualQuaternion.EulerAngles();
        const Matrix3 actualMatrix = actualQuaternion.RotationMatrix();

        CHECK(expected.Equals(actualAngles));
        CHECK(expectedMatrix.Equals(actualMatrix));
    }
}
