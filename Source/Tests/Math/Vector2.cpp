// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

using namespace Urho3D;

TEST_CASE("Vector2 conversion")
{
    const Vector2 axis{1, 2};

    CHECK(axis.ToIntVector2() == IntVector2(1, 2));
    CHECK(axis.ToVector3().Equals(Vector3(1, 2, 0)));
    CHECK(axis.ToIntVector3() == IntVector3(1, 2, 0));
}
