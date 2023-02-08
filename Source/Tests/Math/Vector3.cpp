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

using namespace Urho3D;

TEST_CASE("Vector3 conversion")
{
    const Vector3 value{1, 2, 3};

    CHECK(value.ToVector2().Equals(Vector2(1, 2)));
    CHECK(value.ToIntVector2() == IntVector2(1, 2));
    CHECK(value.ToIntVector3() == IntVector3(1, 2, 3));
}

TEST_CASE("Signed angle between vectors is consistent with quaternion rotation")
{
    const Vector3 axis{0, 0, 1};
    const Vector3 vectorAlpha{1, 0, 0};
    const Vector3 vectorBeta{0, 1, 0};
    const float angleAlphaToBeta = vectorAlpha.SignedAngle(vectorBeta, axis);
    const float angleBetaToAlpha = vectorBeta.SignedAngle(vectorAlpha, axis);

    CHECK(angleAlphaToBeta == -angleBetaToAlpha);

    const Quaternion rotationAlphaToBeta{angleAlphaToBeta, axis};
    const Quaternion rotationBetaToAlpha{angleBetaToAlpha, axis};

    CHECK((rotationAlphaToBeta * vectorAlpha).Equals(vectorBeta, 0.00001f));
    CHECK((rotationBetaToAlpha * vectorBeta).Equals(vectorAlpha, 0.00001f));
}
