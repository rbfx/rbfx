//
// Copyright (c) 2021-2022 the rbfx project.
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
