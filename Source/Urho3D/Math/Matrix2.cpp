// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Math/Matrix2.h"

#include "Urho3D/Container/Str.h"

#include <cstdio>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

const Matrix2 Matrix2::ZERO(
    0.0f, 0.0f,
    0.0f, 0.0f);

const Matrix2 Matrix2::IDENTITY;

Matrix2 Matrix2::Inverse() const
{
    float det = m00_ * m11_ -
                m01_ * m10_;

    float invDet = 1.0f / det;

    return Matrix2(
        m11_, -m01_,
        -m10_, m00_
    ) * invDet;
}

ea::string Matrix2::ToString() const
{
    char tempBuffer[MATRIX_CONVERSION_BUFFER_LENGTH];
    snprintf(tempBuffer, MATRIX_CONVERSION_BUFFER_LENGTH, "%g %g %g %g", m00_, m01_, m10_, m11_);
    return ea::string(tempBuffer);
}
}
