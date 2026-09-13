// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Math/InverseKinematics.h>

using namespace Urho3D;

TEST_CASE("Two-segment FABRIK chain is solved")
{
    IKNode nodes[] = {
        {Vector3{0.0f, 0.0f, 0.0f}, Quaternion::IDENTITY},
        {Vector3{1.0f, 0.0f, 0.0f}, Quaternion::IDENTITY},
        {Vector3{2.0f, 0.0f, 0.0f}, Quaternion::IDENTITY},
    };

    IKFabrikChain chain;
    chain.AddNode(&nodes[0]);
    chain.AddNode(&nodes[1]);
    chain.AddNode(&nodes[2]);
    chain.UpdateLengths();

    chain.Solve(Vector3{1.0f, 1.0f, 0.0f}, IKSettings{});

    CHECK(nodes[0].position_.Equals(Vector3{0.0f, 0.0f, 0.0f}, 0.001f));
    CHECK(nodes[1].position_.Equals(Vector3{1.0f, 0.0f, 0.0f}, 0.001f));
    CHECK(nodes[2].position_.Equals(Vector3{1.0f, 1.0f, 0.0f}, 0.001f));

    CHECK(nodes[0].rotation_.Equals(Quaternion::IDENTITY));
    CHECK(nodes[1].rotation_.Equals(Quaternion{90.0f, Vector3::FORWARD}, 0.001f));
    CHECK(nodes[2].rotation_.Equals(Quaternion{90.0f, Vector3::FORWARD}, 0.001f));
}

TEST_CASE("Two-segment trigonometric chain is solved")
{
    IKNode nodes[] = {
        {Vector3{0.0f, 0.0f, 0.0f}, Quaternion::IDENTITY},
        {Vector3{3.0f, 0.0f, 0.0f}, Quaternion::IDENTITY},
        {Vector3{7.0f, 0.0f, 0.0f}, Quaternion::IDENTITY},
    };

    IKTrigonometricChain chain;
    chain.Initialize(&nodes[0], &nodes[1], &nodes[2]);
    chain.UpdateLengths();

    {
        chain.Solve(Vector3{5.0f, 0.0f, 0.0f}, Vector3::DOWN, Vector3::DOWN, 0.0f, 180.0f);

        CHECK(nodes[0].position_.Equals(Vector3{0.0f, 0.0f, 0.0f}));
        CHECK(nodes[1].position_.Equals(Vector3{1.8f, -2.4f, 0.0f}));
        CHECK(nodes[2].position_.Equals(Vector3{5.0f, 0.0f, 0.0f}));

        CHECK(nodes[0].rotation_.Equals(Quaternion{-53.13f, Vector3::FORWARD}, 0.001f));
        CHECK(nodes[1].rotation_.Equals(Quaternion{-53.13f + 90, Vector3::FORWARD}, 0.001f));
        CHECK(nodes[2].rotation_.Equals(Quaternion{-53.13f + 90, Vector3::FORWARD}, 0.001f));
    }

    {
        chain.Solve(Vector3{7.0f, 0.0f, 0.0f}, Vector3::DOWN, Vector3::DOWN, 0.0f, 180.0f);

        CHECK(nodes[0].position_.Equals(Vector3{0.0f, 0.0f, 0.0f}));
        CHECK(nodes[1].position_.Equals(Vector3{3.0f, 0.0f, 0.0f}));
        CHECK(nodes[2].position_.Equals(Vector3{7.0f, 0.0f, 0.0f}));

        CHECK(nodes[0].rotation_.Equals(Quaternion::IDENTITY));
        CHECK(nodes[1].rotation_.Equals(Quaternion::IDENTITY));
        CHECK(nodes[2].rotation_.Equals(Quaternion::IDENTITY));
    }

    {
        chain.Solve(Vector3{7.0f, 0.0f, 0.0f}, Vector3::DOWN, Vector3::DOWN, 0.0f, 90.0f);

        CHECK(nodes[0].position_.Equals(Vector3{0.0f, 0.0f, 0.0f}));
        CHECK(nodes[1].position_.Equals(Vector3{1.8f, -2.4f, 0.0f}));
        CHECK(nodes[2].position_.Equals(Vector3{5.0f, 0.0f, 0.0f}));

        CHECK(nodes[0].rotation_.Equals(Quaternion{-53.13f, Vector3::FORWARD}, 0.001f));
        CHECK(nodes[1].rotation_.Equals(Quaternion{-53.13f + 90, Vector3::FORWARD}, 0.001f));
        CHECK(nodes[2].rotation_.Equals(Quaternion{-53.13f + 90, Vector3::FORWARD}, 0.001f));
    }

    {
        chain.Solve(Vector3{1.0f, 0.0f, 0.0f}, Vector3::DOWN, Vector3::DOWN, 90.0f, 180.0f);

        CHECK(nodes[0].position_.Equals(Vector3{0.0f, 0.0f, 0.0f}));
        CHECK(nodes[1].position_.Equals(Vector3{1.8f, -2.4f, 0.0f}));
        CHECK(nodes[2].position_.Equals(Vector3{5.0f, 0.0f, 0.0f}));

        CHECK(nodes[0].rotation_.Equals(Quaternion{-53.13f, Vector3::FORWARD}, 0.001f));
        CHECK(nodes[1].rotation_.Equals(Quaternion{-53.13f + 90, Vector3::FORWARD}, 0.001f));
        CHECK(nodes[2].rotation_.Equals(Quaternion{-53.13f + 90, Vector3::FORWARD}, 0.001f));
    }
}
