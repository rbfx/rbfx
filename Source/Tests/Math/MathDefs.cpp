// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Math/MathDefs.h>

TEST_CASE("Zigzag integer encoding and decoding")
{
    static_assert(ZigzagEncode(0) == 0);
    static_assert(ZigzagEncode(-1) == 1);
    static_assert(ZigzagEncode(1) == 2);
    static_assert(ZigzagEncode(-2) == 3);
    static_assert(ZigzagEncode(2) == 4);

    static_assert(0 == ZigzagDecode(0));
    static_assert(-1 == ZigzagDecode(1));
    static_assert(1 == ZigzagDecode(2));
    static_assert(-2 == ZigzagDecode(3));
    static_assert(2 == ZigzagDecode(4));

    static_assert(ZigzagEncode(static_cast<std::int8_t>(-2)) == static_cast<std::uint8_t>(3));
    static_assert(ZigzagEncode(static_cast<std::int8_t>(2)) == static_cast<std::uint8_t>(4));
    static_assert(static_cast<std::int8_t>(-2) == ZigzagDecode(static_cast<std::uint8_t>(3)));
    static_assert(static_cast<std::int8_t>(2) == ZigzagDecode(static_cast<std::uint8_t>(4)));
}
