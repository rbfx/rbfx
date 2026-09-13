// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Math/RandomEngine.h>

TEST_CASE("Engine started multiple times in same process")
{
    {
        auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    }

    {
        auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    }
}

TEST_CASE("Random engine is stable for given seed")
{
    RandomEngine re(12);
    REQUIRE(re.GetUInt() == 579251);
    REQUIRE(re.GetUInt() == 43785880);
    REQUIRE(re.GetUInt() == 464353102);
};
