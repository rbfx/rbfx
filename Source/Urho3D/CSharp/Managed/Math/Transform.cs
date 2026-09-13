// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
