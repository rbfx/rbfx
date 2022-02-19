//
// Copyright (c) 2008-2022 the Urho3D project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Math/Matrix3.h"

#include <cstdio>

#include "../DebugNew.h"

namespace Urho3D
{

const Matrix3 Matrix3::ZERO(
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f);

const Matrix3 Matrix3::IDENTITY;

void Matrix3::FromAngleAxis(float angle, const Vector3& axis)
{
    Vector3 normAxis = axis.Normalized();
    float sinAngle;
    float cosAngle;
    SinCos(angle, sinAngle, cosAngle);
    const float _cosAngle = 1.0f - cosAngle;

    m00_ = cosAngle + normAxis.x_ * normAxis.x_ * _cosAngle;
    m10_ = normAxis.y_ * normAxis.x_ * _cosAngle + normAxis.z_ * sinAngle;
    m20_ = normAxis.z_ * normAxis.x_ * _cosAngle - normAxis.y_ * sinAngle;

    m01_ = normAxis.x_ * normAxis.y_ * _cosAngle - normAxis.z_ * sinAngle;
    m11_ = cosAngle + normAxis.y_ * normAxis.y_ * _cosAngle;
    m21_ = normAxis.z_ * normAxis.y_ * _cosAngle + normAxis.x_ * sinAngle;

    m02_ = normAxis.x_ * normAxis.z_ * _cosAngle + normAxis.y_ * sinAngle;
    m12_ = normAxis.y_ * normAxis.z_ * _cosAngle - normAxis.x_ * sinAngle;
    m22_ = cosAngle + normAxis.z_ * normAxis.z_ * _cosAngle;
}

float Matrix3::Determinant() const
{
    return m00_ * m11_ * m22_ +
           m10_ * m21_ * m02_ +
           m20_ * m01_ * m12_ -
           m20_ * m11_ * m02_ -
           m10_ * m01_ * m22_ -
           m00_ * m21_ * m12_;
}

Matrix3 Matrix3::Inverse() const
{
    const float invDet = 1.0f / Determinant();

    return Matrix3(
        (m11_ * m22_ - m21_ * m12_) * invDet,
        -(m01_ * m22_ - m21_ * m02_) * invDet,
        (m01_ * m12_ - m11_ * m02_) * invDet,
        -(m10_ * m22_ - m20_ * m12_) * invDet,
        (m00_ * m22_ - m20_ * m02_) * invDet,
        -(m00_ * m12_ - m10_ * m02_) * invDet,
        (m10_ * m21_ - m20_ * m11_) * invDet,
        -(m00_ * m21_ - m20_ * m01_) * invDet,
        (m00_ * m11_ - m10_ * m01_) * invDet
    );
}

ea::string Matrix3::ToString() const
{
    char tempBuffer[MATRIX_CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g %g %g %g %g %g %g", m00_, m01_, m02_, m10_, m11_, m12_, m20_, m21_, m22_);
    return ea::string(tempBuffer);
}

}
