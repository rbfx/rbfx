// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Replica/NetworkTime.h>

TEST_CASE("NetworkTime is updated as expected")
{
    SECTION("NetworkTime is initialized")
    {
        CHECK(NetworkTime{} == NetworkTime{NetworkFrame{0}, 0.0f});
        CHECK(NetworkTime{NetworkFrame{1}, 1.25f} == NetworkTime{NetworkFrame{2}, 0.25f});
        CHECK(NetworkTime{NetworkFrame{1}, -0.25f} == NetworkTime{NetworkFrame{0}, 0.75f});
        CHECK(NetworkTime{NetworkFrame{2}, 0.25f}.ToString() == "#2:0.25");
    }

    SECTION("NetworkTime is updated by delta")
    {
        CHECK(NetworkTime{NetworkFrame{1}, 0.25f} + 0.75 == NetworkTime{NetworkFrame{2}, 0.0f});
        CHECK(NetworkTime{NetworkFrame{1}, 0.25f} - 0.75 == NetworkTime{NetworkFrame{0}, 0.5f});
        CHECK(NetworkTime{NetworkFrame{10}, 0.75f} + 23.75 == NetworkTime{NetworkFrame{34}, 0.5f});
        CHECK(NetworkTime{NetworkFrame{10}, 0.25f} - 23.75 == NetworkTime{NetworkFrame{-14}, 0.5f});
        CHECK(NetworkTime{NetworkFrame{-3}, 0.25f} + 2.75 == NetworkTime{});
    }

    SECTION("Delta between NetworkTime-s is evaluated")
    {
        CHECK(NetworkTime{NetworkFrame{1}, 0.25f} - NetworkTime{NetworkFrame{2}, 0.0f} == -0.75);
        CHECK(NetworkTime{NetworkFrame{1}, 0.25f} - NetworkTime{NetworkFrame{0}, 0.5f} == 0.75);
        CHECK(NetworkTime{NetworkFrame{10}, 0.75f} - NetworkTime{NetworkFrame{34}, 0.5f} == -23.75);
        CHECK(NetworkTime{NetworkFrame{10}, 0.25f} - NetworkTime{NetworkFrame{-14}, 0.5f} == 23.75);
        CHECK(NetworkTime{NetworkFrame{-3}, 0.25f} - NetworkTime{} == -2.75);
    }
}
