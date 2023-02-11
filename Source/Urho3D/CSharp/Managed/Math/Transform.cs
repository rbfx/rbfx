//
// Copyright (c) 2017-2023 the rbfx project.
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

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform : IEquatable<Transform>, IApproximateEquatable<Transform>
    {
        public static IEqualityComparer<Transform> ApproximateEqualityComparer => ApproximateEqualityComparer<Transform>.Default;

        public Vector3 Position;
        public Quaternion Rotation;
        public Vector3 Scale;

        public Transform(Vector3 position, Quaternion rotation)
        {
            Position = position;
            Rotation = rotation;
            Scale = Vector3.One;
        }

        public Transform(Vector3 position, Quaternion rotation, Vector3 scale)
        {
            Position = position;
            Rotation = rotation;
            Scale = scale;
        }

        public Matrix3x4 ToMatrix3x4() { return new Matrix3x4(Position, Rotation, Scale); }

        /// Test for equality with another sphere.
        public static bool operator ==(in Transform lhs, in Transform rhs) { return lhs.Equals(rhs); }

        /// Test for inequality with another sphere.
        public static bool operator !=(in Transform lhs, in Transform rhs) { return !lhs.Equals(rhs); }

        public bool Equals(Transform other)
        {
            return Position.Equals(other.Position) &&
                   Rotation.Equals(other.Rotation) &&
                   Scale.Equals(other.Scale);
        }
        public override bool Equals(object obj)
        {
            return obj is Transform other && Equals(other);
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(Position, Rotation, Scale);
        }

        public bool ApproximatelyEquivalent(Transform other, float epsilon = MathDefs.Epsilon)
        {
            return Position.ApproximatelyEquivalent(other.Position, epsilon) &&
                   Rotation.ApproximatelyEquivalent(other.Rotation, epsilon) &&
                   Scale.ApproximatelyEquivalent(other.Scale, epsilon);
        }
    }
}
