//
// Copyright (c) 2017-2021 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

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
