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

#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Quaternion.h>

namespace
{

void TestTransformDecomposition(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
{
    using Catch::Approx;
    static constexpr float rotationMargin = 0.0001f;

    const Matrix3x4 mat{ translation, rotation, scale };

    {
        Vector3 decomposedTranslation;
        Quaternion decomposedRotation;
        Vector3 decomposedScale;
        mat.Decompose(decomposedTranslation, decomposedRotation, decomposedScale);

        CHECK(translation.x_ == Approx(decomposedTranslation.x_).margin(M_EPSILON));
        CHECK(translation.y_ == Approx(decomposedTranslation.y_).margin(M_EPSILON));
        CHECK(translation.z_ == Approx(decomposedTranslation.z_).margin(M_EPSILON));
        CHECK(rotation.Equivalent(decomposedRotation, M_EPSILON));
        CHECK(scale.x_ == Approx(decomposedScale.x_).margin(M_EPSILON));
        CHECK(scale.y_ == Approx(decomposedScale.y_).margin(M_EPSILON));
        CHECK(scale.z_ == Approx(decomposedScale.z_).margin(M_EPSILON));
    }

    {
        const Vector3 decomposedTranslation = mat.Translation();
        const Quaternion decomposedRotation = mat.Rotation();
        const Vector3 decomposedScale = mat.Scale();

        CHECK(translation.x_ == Approx(decomposedTranslation.x_).margin(M_EPSILON));
        CHECK(translation.y_ == Approx(decomposedTranslation.y_).margin(M_EPSILON));
        CHECK(translation.z_ == Approx(decomposedTranslation.z_).margin(M_EPSILON));
        CHECK(rotation.Equivalent(decomposedRotation, M_EPSILON));
        CHECK(scale.x_ == Approx(decomposedScale.x_).margin(M_EPSILON));
        CHECK(scale.y_ == Approx(decomposedScale.y_).margin(M_EPSILON));
        CHECK(scale.z_ == Approx(decomposedScale.z_).margin(M_EPSILON));
    }
}

}

TEST_CASE("Simple transform decomposed")
{
    SECTION("Test identity transform")
    {
        TestTransformDecomposition(Vector3::ZERO, Quaternion::IDENTITY, Vector3::ONE);
    }

    SECTION("Test translation")
    {
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, Quaternion::IDENTITY, Vector3::ONE);
    }

    SECTION("Test small rotation")
    {
        TestTransformDecomposition(Vector3::ZERO, Quaternion{ 33.0f, { 1.0f, 0.0f, 0.0f } }, Vector3::ONE);
        TestTransformDecomposition(Vector3::ZERO, Quaternion{ 33.0f, { 1.0f, -2.0f, 0.1f } }, Vector3::ONE);
    }

    SECTION("Test 180-degree rotation")
    {
        TestTransformDecomposition(Vector3::ZERO, Quaternion{ 180.0f, { 1.0f, 0.0f, 0.0f } }, Vector3::ONE);
        TestTransformDecomposition(Vector3::ZERO, Quaternion{ 180.0f, { 1.0f, -2.0f, 0.1f } }, Vector3::ONE);
    }

    SECTION("Test simple scale")
    {
        TestTransformDecomposition(Vector3::ZERO, Quaternion::IDENTITY, { 0.2f, 1.1f, 3.0f });
    }

    SECTION("Test full transform")
    {
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 33.0f, { 1.0f, 0.0f, 0.0f } }, { 0.2f, 1.1f, 3.0f });
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 33.0f, { 1.0f, -2.0f, 0.1f } }, { 0.2f, 1.1f, 3.0f });
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 180.0f, { 1.0f, 0.0f, 0.0f } }, { 0.2f, 1.1f, 3.0f });
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 180.0f, { 1.0f, -2.0f, 0.1f } }, { 0.2f, 1.1f, 3.0f });
    }
}

TEST_CASE("Mirrored transform decomposed")
{
    SECTION("Test negative scale")
    {
        TestTransformDecomposition(Vector3::ZERO, Quaternion::IDENTITY, { -0.2f, 1.1f, 3.0f });
    }

    SECTION("Test full transform with negative scale")
    {
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 33.0f, { 1.0f, 0.0f, 0.0f } }, { -0.2f, 1.1f, 3.0f });
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 33.0f, { 1.0f, -2.0f, 0.1f } }, { -0.2f, 1.1f, 3.0f });
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 180.0f, { 1.0f, 0.0f, 0.0f } }, { -0.2f, 1.1f, 3.0f });
        TestTransformDecomposition({ 1.1f, -0.1f, 10.5f }, { 180.0f, { 1.0f, -2.0f, 0.1f } }, { -0.2f, 1.1f, 3.0f });
    }
}
