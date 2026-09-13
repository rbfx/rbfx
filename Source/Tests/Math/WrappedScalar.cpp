// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Math/WrappedScalar.h>

TEST_CASE("WrappedScalarRange contains expected values")
{
    using Range = WrappedScalarRange<float>;

    {
        const Range r{1.5f, 2.0f, 1.0f, 2.5f, 0};

        REQUIRE_FALSE(r.ContainsInclusive(1.0f));
        REQUIRE_FALSE(r.ContainsExclusive(1.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(1.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(1.0f));

        REQUIRE(r.ContainsInclusive(1.5f));
        REQUIRE_FALSE(r.ContainsExclusive(1.5f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(1.5f));
        REQUIRE(r.ContainsExcludingEnd(1.5f));

        REQUIRE(r.ContainsInclusive(1.8f));
        REQUIRE(r.ContainsExclusive(1.8f));
        REQUIRE(r.ContainsExcludingBegin(1.8f));
        REQUIRE(r.ContainsExcludingEnd(1.8f));

        REQUIRE(r.ContainsInclusive(2.0f));
        REQUIRE_FALSE(r.ContainsExclusive(2.0f));
        REQUIRE(r.ContainsExcludingBegin(2.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(2.0f));

        REQUIRE_FALSE(r.ContainsInclusive(2.5f));
        REQUIRE_FALSE(r.ContainsExclusive(2.5f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(2.5f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(2.5f));
    }

    {
        const Range r{2.0f, 1.5f, 1.0f, 2.5f, 0};

        REQUIRE_FALSE(r.ContainsInclusive(1.0f));
        REQUIRE_FALSE(r.ContainsExclusive(1.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(1.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(1.0f));

        REQUIRE(r.ContainsInclusive(1.5f));
        REQUIRE_FALSE(r.ContainsExclusive(1.5f));
        REQUIRE(r.ContainsExcludingBegin(1.5f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(1.5f));

        REQUIRE(r.ContainsInclusive(1.8f));
        REQUIRE(r.ContainsExclusive(1.8f));
        REQUIRE(r.ContainsExcludingBegin(1.8f));
        REQUIRE(r.ContainsExcludingEnd(1.8f));

        REQUIRE(r.ContainsInclusive(2.0f));
        REQUIRE_FALSE(r.ContainsExclusive(2.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(2.0f));
        REQUIRE(r.ContainsExcludingEnd(2.0f));

        REQUIRE_FALSE(r.ContainsInclusive(2.5f));
        REQUIRE_FALSE(r.ContainsExclusive(2.5f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(2.5f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(2.5f));
    }

    {
        const Range r{1.5f, 2.0f, 1.0f, 2.5f, 1};

        REQUIRE_FALSE(r.ContainsInclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(0.8f));

        REQUIRE(r.ContainsInclusive(1.0f));
        REQUIRE(r.ContainsExclusive(1.0f));
        REQUIRE(r.ContainsExcludingBegin(1.0f));
        REQUIRE(r.ContainsExcludingEnd(1.0f));

        REQUIRE(r.ContainsInclusive(1.5f));
        REQUIRE(r.ContainsExclusive(1.5f));
        REQUIRE(r.ContainsExcludingBegin(1.5f));
        REQUIRE(r.ContainsExcludingEnd(1.5f));

        REQUIRE(r.ContainsInclusive(1.8f));
        REQUIRE(r.ContainsExclusive(1.8f));
        REQUIRE(r.ContainsExcludingBegin(1.8f));
        REQUIRE(r.ContainsExcludingEnd(1.8f));

        REQUIRE(r.ContainsInclusive(2.0f));
        REQUIRE(r.ContainsExclusive(2.0f));
        REQUIRE(r.ContainsExcludingBegin(2.0f));
        REQUIRE(r.ContainsExcludingEnd(2.0f));

        REQUIRE(r.ContainsInclusive(2.5f));
        REQUIRE(r.ContainsExclusive(2.5f));
        REQUIRE(r.ContainsExcludingBegin(2.5f));
        REQUIRE(r.ContainsExcludingEnd(2.5f));

        REQUIRE_FALSE(r.ContainsInclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(3.0f));
    }

    {
        const Range r{2.0f, 1.5f, 1.0f, 2.5f, 1};

        REQUIRE_FALSE(r.ContainsInclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(0.8f));

        REQUIRE(r.ContainsInclusive(1.0f));
        REQUIRE(r.ContainsExclusive(1.0f));
        REQUIRE(r.ContainsExcludingBegin(1.0f));
        REQUIRE(r.ContainsExcludingEnd(1.0f));

        REQUIRE(r.ContainsInclusive(1.5f));
        REQUIRE_FALSE(r.ContainsExclusive(1.5f));
        REQUIRE(r.ContainsExcludingBegin(1.5f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(1.5f));

        REQUIRE_FALSE(r.ContainsInclusive(1.8f));
        REQUIRE_FALSE(r.ContainsExclusive(1.8f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(1.8f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(1.8f));

        REQUIRE(r.ContainsInclusive(2.0f));
        REQUIRE_FALSE(r.ContainsExclusive(2.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(2.0f));
        REQUIRE(r.ContainsExcludingEnd(2.0f));

        REQUIRE(r.ContainsInclusive(2.5f));
        REQUIRE(r.ContainsExclusive(2.5f));
        REQUIRE(r.ContainsExcludingBegin(2.5f));
        REQUIRE(r.ContainsExcludingEnd(2.5f));

        REQUIRE_FALSE(r.ContainsInclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(3.0f));
    }

    {
        const Range r{1.5f, 2.0f, 1.0f, 2.5f, -1};

        REQUIRE_FALSE(r.ContainsInclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(0.8f));

        REQUIRE(r.ContainsInclusive(1.0f));
        REQUIRE(r.ContainsExclusive(1.0f));
        REQUIRE(r.ContainsExcludingBegin(1.0f));
        REQUIRE(r.ContainsExcludingEnd(1.0f));

        REQUIRE(r.ContainsInclusive(1.5f));
        REQUIRE_FALSE(r.ContainsExclusive(1.5f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(1.5f));
        REQUIRE(r.ContainsExcludingEnd(1.5f));

        REQUIRE_FALSE(r.ContainsInclusive(1.8f));
        REQUIRE_FALSE(r.ContainsExclusive(1.8f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(1.8f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(1.8f));

        REQUIRE(r.ContainsInclusive(2.0f));
        REQUIRE_FALSE(r.ContainsExclusive(2.0f));
        REQUIRE(r.ContainsExcludingBegin(2.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(2.0f));

        REQUIRE(r.ContainsInclusive(2.5f));
        REQUIRE(r.ContainsExclusive(2.5f));
        REQUIRE(r.ContainsExcludingBegin(2.5f));
        REQUIRE(r.ContainsExcludingEnd(2.5f));

        REQUIRE_FALSE(r.ContainsInclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(3.0f));
    }

    {
        const Range r{2.0f, 1.5f, 1.0f, 2.5f, -1};

        REQUIRE_FALSE(r.ContainsInclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExclusive(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(0.8f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(0.8f));

        REQUIRE(r.ContainsInclusive(1.0f));
        REQUIRE(r.ContainsExclusive(1.0f));
        REQUIRE(r.ContainsExcludingBegin(1.0f));
        REQUIRE(r.ContainsExcludingEnd(1.0f));

        REQUIRE(r.ContainsInclusive(1.5f));
        REQUIRE(r.ContainsExclusive(1.5f));
        REQUIRE(r.ContainsExcludingBegin(1.5f));
        REQUIRE(r.ContainsExcludingEnd(1.5f));

        REQUIRE(r.ContainsInclusive(1.8f));
        REQUIRE(r.ContainsExclusive(1.8f));
        REQUIRE(r.ContainsExcludingBegin(1.8f));
        REQUIRE(r.ContainsExcludingEnd(1.8f));

        REQUIRE(r.ContainsInclusive(2.0f));
        REQUIRE(r.ContainsExclusive(2.0f));
        REQUIRE(r.ContainsExcludingBegin(2.0f));
        REQUIRE(r.ContainsExcludingEnd(2.0f));

        REQUIRE(r.ContainsInclusive(2.5f));
        REQUIRE(r.ContainsExclusive(2.5f));
        REQUIRE(r.ContainsExcludingBegin(2.5f));
        REQUIRE(r.ContainsExcludingEnd(2.5f));

        REQUIRE_FALSE(r.ContainsInclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExclusive(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingBegin(3.0f));
        REQUIRE_FALSE(r.ContainsExcludingEnd(3.0f));
    }

}

TEST_CASE("WrappedScalar is wrapped on update and returns expected ranges")
{
    using Value = WrappedScalar<float>;
    using Range = WrappedScalarRange<float>;

    Value scalar(0.0f, 1.0f, 2.5f);

    REQUIRE(scalar.Value() == 1.0f);

    REQUIRE(scalar.UpdateWrapped(0.5f) == Range{1.0f, 1.5f, 1.0f, 2.5f, 0});
    REQUIRE(scalar.Value() == 1.5f);

    REQUIRE(scalar.UpdateWrapped(1.5f) == Range{1.5f, 1.5f, 1.0f, 2.5f, 1});
    REQUIRE(scalar.Value() == 1.5f);

    REQUIRE(scalar.UpdateWrapped(-1.0f) == Range{1.5f, 2.0f, 1.0f, 2.5f, -1});
    REQUIRE(scalar.Value() == 2.0f);

    REQUIRE(scalar.UpdateWrapped(3.25f) == Range{2.0f, 2.25f, 1.0f, 2.5f, 2});
    REQUIRE(scalar.Value() == 2.25f);

    REQUIRE(scalar.UpdateWrapped(-3.25f) == Range{2.25f, 2.0f, 1.0f, 2.5f, -2});
    REQUIRE(scalar.Value() == 2.0f);
}

TEST_CASE("WrappedScalar is clamped on update and returns expected ranges")
{
    using Value = WrappedScalar<float>;
    using Range = WrappedScalarRange<float>;

    Value scalar(0.0f, 1.0f, 2.5f);

    REQUIRE(scalar == Value{}.MinMaxClamped(1.0f, 2.5f));

    REQUIRE(scalar.Value() == 1.0f);

    REQUIRE(scalar.UpdateClamped(0.5f) == Range{1.0f, 1.5f, 1.0f, 2.5f, 0});
    REQUIRE(scalar.Value() == 1.5f);

    REQUIRE(scalar.UpdateClamped(1.5f) == Range{1.5f, 2.5f, 1.0f, 2.5f, 0});
    REQUIRE(scalar.Value() == 2.5f);

    REQUIRE(scalar.UpdateClamped(-1.0f) == Range{2.5f, 1.5f, 1.0f, 2.5f, 0});
    REQUIRE(scalar.Value() == 1.5f);

    REQUIRE(scalar.UpdateClamped(-3.0f) == Range{1.5f, 1.0f, 1.0f, 2.5f, 0});
    REQUIRE(scalar.Value() == 1.0f);
}
