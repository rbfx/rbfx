// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
