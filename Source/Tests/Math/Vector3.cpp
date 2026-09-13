// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

using namespace Urho3D;

TEST_CASE("Vector3 conversion")
{
    const Vector3 value{1, 2, 3};

    CHECK(value.ToVector2().Equals(Vector2(1, 2)));
    CHECK(value.ToIntVector2() == IntVector2(1, 2));
    CHECK(value.ToIntVector3() == IntVector3(1, 2, 3));
}

TEST_CASE("Signed angle between vectors is consistent with quaternion rotation")
{
    const Vector3 axis{0, 0, 1};
    const Vector3 vectorAlpha{1, 0, 0};
    const Vector3 vectorBeta{0, 1, 0};
    const float angleAlphaToBeta = vectorAlpha.SignedAngle(vectorBeta, axis);
    const float angleBetaToAlpha = vectorBeta.SignedAngle(vectorAlpha, axis);

    CHECK(angleAlphaToBeta == -angleBetaToAlpha);

    const Quaternion rotationAlphaToBeta{angleAlphaToBeta, axis};
    const Quaternion rotationBetaToAlpha{angleBetaToAlpha, axis};

    CHECK((rotationAlphaToBeta * vectorAlpha).Equals(vectorBeta, 0.00001f));
    CHECK((rotationBetaToAlpha * vectorBeta).Equals(vectorAlpha, 0.00001f));
}
