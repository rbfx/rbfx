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
