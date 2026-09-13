// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

using namespace Urho3D;

TEST_CASE("IntVector2 conversion")
{
    const IntVector2 value{1, 2};

    CHECK(value.ToVector2().Equals(Vector2(1, 2)));
    CHECK(value.ToVector3().Equals(Vector3(1, 2, 0)));
    CHECK(value.ToIntVector3() == IntVector3(1, 2, 0));
}
