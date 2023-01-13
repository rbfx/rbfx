//
// Copyright (c) 2017-2020 the rbfx project.
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

#pragma once

#include "../Math/Matrix3x4.h"
#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"

namespace Urho3D
{

/// Position, rotation and scale in 3D.
/// TODO: Expand it into something more user-friendly.
struct Transform
{
    Vector3 position_;
    Quaternion rotation_;
    Vector3 scale_{Vector3::ONE};

    Matrix3x4 ToMatrix3x4() const { return Matrix3x4(position_, rotation_, scale_); };

    /// Return inverse transform. It is not precise for non-uniform scale.
    Transform Inverse() const
    {
        Transform ret;
        ret.rotation_ = rotation_.Inverse();
        ret.scale_ = Vector3::ONE / scale_;
        ret.position_ = -(ret.rotation_ * position_) * ret.scale_;
        return ret;
    }

    /// Return transform multiplied by another transform.
    Transform operator *(const Transform& rhs) const
    {
        Transform ret;
        ret.rotation_ = rotation_ * rhs.rotation_;
        ret.scale_ = scale_ * rhs.scale_;
        ret.position_ = position_ + rotation_ * (rhs.position_ * scale_);
        return ret;
    }

    /// Return position multiplied by the transform.
    Vector3 operator *(const Vector3& rhs) const
    {
        return position_ + rotation_ * (rhs * scale_);
    }

    /// Return rotation multiplied by the transform.
    Quaternion operator *(const Quaternion& rhs) const
    {
        return rotation_ * rhs;
    }
};

}
