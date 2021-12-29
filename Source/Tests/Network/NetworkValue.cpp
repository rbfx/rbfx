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

#include <Urho3D/Network/NetworkValue.h>

TEST_CASE("NetworkValue is updated and sampled")
{
    NetworkValue<float> v;
    v.Resize(5);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE_FALSE(v.GetRaw(2));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    v.Append(2, 1000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 1000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 1000.0f);
    REQUIRE(v.GetClosestRaw(2) == 1000.0f);
    REQUIRE(v.GetClosestRaw(5) == 1000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 1000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 1000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 1000.0f);

    v.Append(2, 2000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 1000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 1000.0f);
    REQUIRE(v.GetClosestRaw(2) == 1000.0f);
    REQUIRE(v.GetClosestRaw(5) == 1000.0f);

    v.Replace(2, 2000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 2000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 2000.0f);
    REQUIRE(v.GetClosestRaw(2) == 2000.0f);
    REQUIRE(v.GetClosestRaw(5) == 2000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 2000.0f);

    v.Append(4, 4000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 2000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE(v.GetRaw(4) == 4000.0f);
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 2000.0f);
    REQUIRE(v.GetClosestRaw(2) == 2000.0f);
    REQUIRE(v.GetClosestRaw(3) == 2000.0f);
    REQUIRE(v.GetClosestRaw(4) == 4000.0f);
    REQUIRE(v.GetClosestRaw(5) == 4000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.0f}) == 4000.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.5f}) == 4000.0f);

    v.Append(3, 3000.0f);
    v.Append(5, 5000.0f);
    v.Append(6, 6000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 2000.0f);
    REQUIRE(v.GetRaw(3) == 3000.0f);
    REQUIRE(v.GetRaw(4) == 4000.0f);
    REQUIRE(v.GetRaw(5) == 5000.0f);
    REQUIRE(v.GetRaw(6) == 6000.0f);

    REQUIRE(v.GetClosestRaw(5) == 5000.0f);
    REQUIRE(v.GetClosestRaw(6) == 6000.0f);
    REQUIRE(v.GetClosestRaw(7) == 6000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 2500.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.0f}) == 3000.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.5f}) == 3500.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.0f}) == 4000.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.5f}) == 4500.0f);

    REQUIRE(v.SampleValid(NetworkTime{5, 0.75f}) == 5750.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.0f}) == 6000.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.5f}) == 6000.0f);

    v.Append(9, 9000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE_FALSE(v.GetRaw(2));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE(v.GetRaw(5) == 5000.0f);
    REQUIRE(v.GetRaw(6) == 6000.0f);
    REQUIRE_FALSE(v.GetRaw(7));
    REQUIRE_FALSE(v.GetRaw(8));
    REQUIRE(v.GetRaw(9) == 9000.0f);

    REQUIRE(v.GetClosestRaw(4) == 5000.0f);
    REQUIRE(v.GetClosestRaw(5) == 5000.0f);
    REQUIRE(v.GetClosestRaw(6) == 6000.0f);
    REQUIRE(v.GetClosestRaw(7) == 6000.0f);
    REQUIRE(v.GetClosestRaw(8) == 6000.0f);
    REQUIRE(v.GetClosestRaw(9) == 9000.0f);
    REQUIRE(v.GetClosestRaw(10) == 9000.0f);

    REQUIRE(v.SampleValid(NetworkTime{4, 0.5f}) == 5000.0f);
    REQUIRE(v.SampleValid(NetworkTime{5, 0.0f}) == 5000.0f);
    REQUIRE(v.SampleValid(NetworkTime{5, 0.5f}) == 5500.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.0f}) == 6000.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.5f}) == 6000.0f);
}

TEST_CASE("NetworkValue is repaired on demand")
{
    const unsigned maxPenalty = 3;

    NetworkValue<float> v;
    v.Resize(10);

    v.Append(5, 5000.0f);
    v.Append(10, 10000.0f);
    v.Append(12, 12000.0f);

    REQUIRE_FALSE(v.RepairAndSample(NetworkTime{4, 0.0f}, maxPenalty));
    REQUIRE_FALSE(v.RepairAndSample(NetworkTime{4, 0.5f}, maxPenalty));
    REQUIRE(v.RepairAndSample(NetworkTime{5, 0.0f}, maxPenalty) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{5, 0.5f}, maxPenalty) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{6, 0.0f}, maxPenalty) == 5000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{8, 0.0f}, maxPenalty) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{8, 0.5f}, maxPenalty) == 6250.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{9, 0.0f}, maxPenalty) == 7500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{9, 0.5f}, maxPenalty) == 8750.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{10, 0.0f}, maxPenalty) == 10000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{10, 0.5f}, maxPenalty) == 10500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{11, 0.0f}, maxPenalty) == 11000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{11, 0.5f}, maxPenalty) == 11500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{12, 0.0f}, maxPenalty) == 12000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{12, 0.5f}, maxPenalty) == 12500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{13, 0.0f}, maxPenalty) == 13000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{13, 0.5f}, maxPenalty) == 13500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{14, 0.0f}, maxPenalty) == 14000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{14, 0.5f}, maxPenalty) == 14000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{15, 0.0f}, maxPenalty) == 14000.0f);

}
