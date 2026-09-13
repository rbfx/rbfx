// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

using namespace Urho3D;

TEST_CASE("IntVector3 conversion")
{
    const IntVector3 value{1, 2, 3};

    CHECK(value.ToVector2().Equals(Vector2(1, 2)));
    CHECK(value.ToVector3().Equals(Vector3(1, 2, 3)));
    CHECK(value.ToIntVector2() == IntVector2(1, 2));
}
