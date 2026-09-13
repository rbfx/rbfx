// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Replica/TickSynchronizer.h>

TEST_CASE("Different clocks are synchronized on client")
{
    TickSynchronizer sync(2, false);
    sync.SetFollowerFrequency(4);

    SECTION("Normal update")
    {
        REQUIRE(sync.Synchronize(0.0f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 1);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.125f);

        sync.Update(0.25f);
        REQUIRE(sync.GetPendingFollowerTicks() == 1);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.125f);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);
    }

    SECTION("Update with small overtime")
    {
        REQUIRE(sync.Synchronize(0.125f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 1);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.125f);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 1);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);

        sync.Update(0.25f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);
    }

    SECTION("Update with big overtime")
    {
        REQUIRE(sync.Synchronize(0.375f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.125f);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);
    }

    SECTION("Update with debt on synchronization")
    {
        REQUIRE(sync.Synchronize(0.0f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 1);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);

        REQUIRE(sync.Synchronize(0.0f) == 1);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);
        REQUIRE(sync.GetFollowerAccumulatedTime() == 0.0f);
    }
}

TEST_CASE("Different clocks are synchronized on server")
{
    TickSynchronizer sync(2, true);
    sync.SetFollowerFrequency(4);

    SECTION("Normal update")
    {
        REQUIRE(sync.Synchronize(0.0f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);

        sync.Update(0.25f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
    }

    SECTION("Update with small overtime")
    {
        REQUIRE(sync.Synchronize(0.125f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);

        sync.Update(0.25f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
    }

    SECTION("Update with big overtime")
    {
        REQUIRE(sync.Synchronize(0.375f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);

        sync.Update(0.125f);
        REQUIRE(sync.GetPendingFollowerTicks() == 0);
    }

    SECTION("Update with debt on synchronization")
    {
        REQUIRE(sync.Synchronize(0.0f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);

        REQUIRE(sync.Synchronize(0.0f) == 0);
        REQUIRE(sync.GetPendingFollowerTicks() == 2);
    }
}
