//
// Copyright (c) 2024-2024 the rbfx project.
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

#include <EASTL/variant.h>

using namespace Urho3D;

namespace
{
    struct TwoPlanesTestData
    {
        Vector3 point_;
        Vector3 n1_;
        Vector3 n2_;
    };
    struct ThreePlanesTestData
    {
        Vector3 point_;
        Vector3 n1_;
        Vector3 n2_;
        Vector3 n3_;
    };
} // namespace

TEST_CASE("Plane GetPoint")
{
    const Plane plane{Vector3{0, 0, 1}, Vector3{0, 0, 1}};
    const auto point = plane.GetPoint();
    CHECK(point.Equals(Vector3{0, 0, 1}, 1e-6f));
}

TEST_CASE("Two planes intersect")
{
    ea::array<TwoPlanesTestData,3> testsData{
        TwoPlanesTestData{Vector3{1, 2, 3}, Vector3{1, 0, 0}, Vector3{0, 1, 0}},
        TwoPlanesTestData{Vector3{1, 2, 3}, Vector3{0, 1, 0}, Vector3{0, 0, 1}},
        TwoPlanesTestData{Vector3{1, 2, 3}, Vector3{0, 0, 1}, Vector3{1, 0, 0}},
    };

    for (auto& data : testsData)
    {
        const Plane planeA{data.n1_, data.point_};
        const Plane planeB{data.n2_, data.point_};
        auto ray = planeA.Intersect(planeB);
        CHECK(Equals(planeA.Distance(ray.origin_), 0.0f, 1e-6f));
        CHECK(Equals(planeB.Distance(ray.origin_), 0.0f, 1e-6f));
        CHECK(Equals(ray.direction_.DotProduct(planeA.normal_), 0.0f, 1e-6f));
        CHECK(Equals(ray.direction_.DotProduct(planeB.normal_), 0.0f, 1e-6f));
    }
}

TEST_CASE("Three planes intersect")
{
    ea::array<ThreePlanesTestData, 3> testsData
    {
        ThreePlanesTestData{Vector3{1, 2, 3}, Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}},
        ThreePlanesTestData{Vector3{1, 2, 3}, Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{1, 1, 1}},
        ThreePlanesTestData{Vector3{0, 0, 0}, Vector3{-1, 0, 0}, Vector3{0, 1, 0}, Vector3{1, 1, 1}}
    };

    for (auto& data : testsData)
    {
        const Plane planeA{data.n1_, data.point_};
        const Plane planeB{data.n2_, data.point_};
        const Plane planeC{data.n3_, data.point_};
        auto point = planeA.Intersect(planeB, planeC);

        CHECK(Equals(planeA.Distance(point), 0.0f, 1e-6f));
        CHECK(Equals(planeB.Distance(point), 0.0f, 1e-6f));
        CHECK(Equals(planeC.Distance(point), 0.0f, 1e-6f));

        CHECK(point.Equals(data.point_, 1e-6f));
    }
}
