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
