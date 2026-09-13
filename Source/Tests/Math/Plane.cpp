// Copyright (c) 2024-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
