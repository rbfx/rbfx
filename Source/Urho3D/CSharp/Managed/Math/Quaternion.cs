//
// Copyright (c) 2008-2019 the Urho3D project.
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

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    /// Rotation represented as a four-dimensional normalized vector.
    [StructLayout(LayoutKind.Sequential)]
    public struct Quaternion : IEquatable<Quaternion>
    {
        /// Construct from values, or identity quaternion by default.
        public Quaternion(float w = 1, float x = 0, float y = 0, float z = 0)
        {
            W = w;
            X = x;
            Y = y;
            Z = z;
        }

        /// Construct from a float array.
        public Quaternion(IReadOnlyList<float> data)
        {
            W = data[0];
            X = data[1];
            Y = data[2];
            Z = data[3];
        }

        /// Construct from an angle (in degrees) and axis.
        public Quaternion(float angle, in Vector3 axis)
        {
            W = X = Y = Z = 0;
            FromAngleAxis(angle, axis);
        }

        /// Construct from an angle (in degrees, for Urho2D).
        public Quaternion(float angle)
        {
            W = X = Y = Z = 0;
            FromAngleAxis(angle, Vector3.Forward);
        }

        /// Construct from Euler angles (in degrees.)
        public Quaternion(float x, float y, float z)
        {
            W = X = Y = Z = 0;
            FromEulerAngles(x, y, z);
        }

        /// Construct from Euler angles (in degrees.)
        public Quaternion(Vector3 angles)
        {
            W = X = Y = Z = 0;
            FromEulerAngles(angles.X, angles.Y, angles.Z);
        }

        /// Construct from the rotation difference between two direction vectors.
        public Quaternion(in Vector3 start, in Vector3 end)
        {
            W = X = Y = Z = 0;
            FromRotationTo(start, end);
        }

        /// Construct from orthonormal axes.
        public Quaternion(in Vector3 xAxis, in Vector3 yAxis, in Vector3 zAxis)
        {
            W = X = Y = Z = 0;
            FromAxes(xAxis, yAxis, zAxis);
        }

        /// Construct from a rotation matrix.
        public Quaternion(in Matrix3 matrix)
        {
            W = X = Y = Z = 0;
            FromRotationMatrix(matrix);
        }

        /// Test for equality with another quaternion without epsilon.
        public static bool operator ==(in Quaternion lhs, in Quaternion rhs)
        {
            return lhs.W == rhs.W && lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
        }

        /// Test for inequality with another quaternion without epsilon.
        public static bool operator !=(in Quaternion lhs, in Quaternion rhs)
        {
            return !(lhs == rhs);
        }

        /// Multiply with a scalar.
        public static Quaternion operator *(in Quaternion lhs, float rhs)
        {
            return new Quaternion(lhs.W * rhs, lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs);
        }

        /// Return negation.
        public static Quaternion operator -(in Quaternion rhs)
        {
            return new Quaternion(-rhs.W, -rhs.X, -rhs.Y, -rhs.Z);
        }

        /// Add a quaternion.
        public static Quaternion operator +(in Quaternion lhs, in Quaternion rhs)
        {
            return new Quaternion(lhs.W + rhs.W, lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
        }

        /// Subtract a quaternion.
        public static Quaternion operator -(in Quaternion lhs, in Quaternion rhs)
        {
            return new Quaternion(lhs.W - rhs.W, lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
        }

        /// Multiply a quaternion.
        public static Quaternion operator *(in Quaternion lhs, in Quaternion rhs)
        {
            return new Quaternion(
                lhs.W * rhs.W - lhs.X * rhs.X - lhs.Y * rhs.Y - lhs.Z * rhs.Z,
                lhs.W * rhs.X + lhs.X * rhs.W + lhs.Y * rhs.Z - lhs.Z * rhs.Y,
                lhs.W * rhs.Y + lhs.Y * rhs.W + lhs.Z * rhs.X - lhs.X * rhs.Z,
                lhs.W * rhs.Z + lhs.Z * rhs.W + lhs.X * rhs.Y - lhs.Y * rhs.X
            );
        }

        /// Multiply a Vector3.
        public static Vector3 operator *(in Quaternion lhs, in Vector3 rhs)
        {
            Vector3 qVec = new Vector3(lhs.X, lhs.Y, lhs.Z);
            Vector3 cross1 = qVec.CrossProduct(rhs);
            Vector3 cross2 = qVec.CrossProduct(cross1);

            return rhs + 2.0f * (cross1 * lhs.W + cross2);
        }

        /// Define from an angle (in degrees) and axis.
        public void FromAngleAxis(float angle, in Vector3 axis)
        {
            Vector3 normAxis = axis.Normalized;
            angle = MathDefs.DegreesToRadians2(angle);
            float sinAngle = (float) Math.Sin(angle);
            float cosAngle = (float) Math.Cos(angle);

            W = cosAngle;
            X = normAxis.X * sinAngle;
            Y = normAxis.Y * sinAngle;
            Z = normAxis.Z * sinAngle;
        }

        /// Define from Euler angles (in degrees.)
        public void FromEulerAngles(float x, float y, float z)
        {
            // Order of rotations: Z first, then X, then Y (mimics typical FPS camera with gimbal lock at top/bottom)
            x = MathDefs.DegreesToRadians2(x);
            y = MathDefs.DegreesToRadians2(y);
            z = MathDefs.DegreesToRadians2(z);
            float sinX = (float) Math.Sin(x);
            float cosX = (float) Math.Cos(x);
            float sinY = (float) Math.Sin(y);
            float cosY = (float) Math.Cos(y);
            float sinZ = (float) Math.Sin(z);
            float cosZ = (float) Math.Cos(z);

            W = cosY * cosX * cosZ + sinY * sinX * sinZ;
            X = cosY * sinX * cosZ + sinY * cosX * sinZ;
            Y = sinY * cosX * cosZ - cosY * sinX * sinZ;
            Z = cosY * cosX * sinZ - sinY * sinX * cosZ;
        }

        /// Define from the rotation difference between two direction vectors.
        public void FromRotationTo(in Vector3 start, in Vector3 end)
        {
            Vector3 normStart = start.Normalized;
            Vector3 normEnd = end.Normalized;
            float d = normStart.DotProduct(normEnd);

            if (d > -1.0f + float.Epsilon)
            {
                Vector3 c = normStart.CrossProduct(normEnd);
                float s = (float) Math.Sqrt((1.0f + d) * 2.0f);
                float invS = 1.0f / s;

                X = c.X * invS;
                Y = c.Y * invS;
                Z = c.Z * invS;
                W = 0.5f * s;
            }
            else
            {
                Vector3 axis = Vector3.Right.CrossProduct(normStart);
                if (axis.Length < float.Epsilon)
                    axis = Vector3.Up.CrossProduct(normStart);

                FromAngleAxis(180, axis);
            }
        }

        /// Define from orthonormal axes.
        public void FromAxes(in Vector3 xAxis, in Vector3 yAxis, in Vector3 zAxis)
        {
            Matrix3 matrix = new Matrix3(
                xAxis.X, yAxis.X, zAxis.X,
                xAxis.Y, yAxis.Y, zAxis.Y,
                xAxis.Z, yAxis.Z, zAxis.Z
            );

            FromRotationMatrix(matrix);
        }

        /// Define from a rotation matrix.
        public void FromRotationMatrix(in Matrix3 matrix)
        {
            float t = matrix.M00 + matrix.M11 + matrix.M22;

            if (t > 0.0f)
            {
                float invS = 0.5f / (float) Math.Sqrt(1.0f + t);

                X = (matrix.M21 - matrix.M12) * invS;
                Y = (matrix.M02 - matrix.M20) * invS;
                Z = (matrix.M10 - matrix.M01) * invS;
                W = 0.25f / invS;
            }
            else
            {
                if (matrix.M00 > matrix.M11 && matrix.M00 > matrix.M22)
                {
                    float invS = 0.5f / (float) Math.Sqrt(1.0f + matrix.M00 - matrix.M11 - matrix.M22);

                    X = 0.25f / invS;
                    Y = (matrix.M01 + matrix.M10) * invS;
                    Z = (matrix.M20 + matrix.M02) * invS;
                    W = (matrix.M21 - matrix.M12) * invS;
                }
                else if (matrix.M11 > matrix.M22)
                {
                    float invS = 0.5f / (float) Math.Sqrt(1.0f + matrix.M11 - matrix.M00 - matrix.M22);

                    X = (matrix.M01 + matrix.M10) * invS;
                    Y = 0.25f / invS;
                    Z = (matrix.M12 + matrix.M21) * invS;
                    W = (matrix.M02 - matrix.M20) * invS;
                }
                else
                {
                    float invS = 0.5f / (float) Math.Sqrt(1.0f + matrix.M22 - matrix.M00 - matrix.M11);

                    X = (matrix.M02 + matrix.M20) * invS;
                    Y = (matrix.M12 + matrix.M21) * invS;
                    Z = 0.25f / invS;
                    W = (matrix.M10 - matrix.M01) * invS;
                }
            }
        }

        /// Define from a direction to look in and an up direction. Return true if successful, or false if would result in a NaN, in which case the current value remains.
        public bool FromLookRotation(in Vector3 direction, in Vector3? up = null)
        {
            Quaternion ret;
            Vector3 forward = direction.Normalized;

            Vector3 v = forward.CrossProduct(up.GetValueOrDefault(Vector3.Up));
            // If direction & up are parallel and crossproduct becomes zero, use FromRotationTo() fallback
            if (v.LengthSquared >= float.Epsilon)
            {
                v.Normalize();
                Vector3 up2 = v.CrossProduct(forward);
                Vector3 right = up2.CrossProduct(forward);
                ret = new Quaternion(right, up2, forward);
            }
            else
                ret = new Quaternion(Vector3.Forward, forward);

            if (!ret.IsNaN)
            {
                this = ret;
                return true;
            }
            else
                return false;
        }

        /// Normalize to unit length.
        public void Normalize()
        {
            float lenSquared = LengthSquared;
            if (!MathDefs.Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
            {
                float invLen = 1.0f / (float) Math.Sqrt(lenSquared);
                W *= invLen;
                X *= invLen;
                Y *= invLen;
                Z *= invLen;
            }
        }

        /// Return normalized to unit length.
        public Quaternion Normalized
        {
            get
            {
                float lenSquared = LengthSquared;
                if (!MathDefs.Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
                {
                    float invLen = 1.0f / (float) Math.Sqrt(lenSquared);
                    return this * invLen;
                }
                else
                    return this;
            }
        }

        /// Return inverse.
        public Quaternion Inversed
        {
            get
            {
                float lenSquared = LengthSquared;
                if (lenSquared == 1.0f)
                    return Conjugated;
                else if (lenSquared >= float.Epsilon)
                    return Conjugated * (1.0f / lenSquared);
                else
                    return IDENTITY;
            }
        }

        /// Return squared length.
        public float LengthSquared => W * W + X * X + Y * Y + Z * Z;

        /// Calculate dot product.
        public float DotProduct(in Quaternion rhs)
        {
            return W * rhs.W + X * rhs.X + Y * rhs.Y + Z * rhs.Z;
        }

        /// Test for equality with another quaternion with epsilon.
        public bool Equals(Quaternion rhs)
        {
            return MathDefs.Equals(W, rhs.W) && MathDefs.Equals(X, rhs.X) && MathDefs.Equals(Y, rhs.Y) &&
                   MathDefs.Equals(Z, rhs.Z);
        }

        /// Return whether is NaN.
        public bool IsNaN => float.IsNaN(W) || float.IsNaN(X) || float.IsNaN(Y) || float.IsNaN(Z);

        /// Return conjugate.
        public Quaternion Conjugated => new Quaternion(W, -X, -Y, -Z);

        /// Return Euler angles in degrees.
        public Vector3 EulerAngles
        {
            get
            {
                // Derivation from http://www.geometrictools.com/Documentation/EulerAngles.pdf
                // Order of rotations: Z first, then X, then Y
                float check = 2.0f * (-Y * Z + W * X);

                if (check < -0.995f)
                {
                    return new Vector3(-90.0f, 0.0f,
                        MathDefs.RadiansToDegrees(-(float) Math.Atan2(2.0f * (X * Z - W * Y),
                            1.0f - 2.0f * (Y * Y + Z * Z))));
                }
                else if (check > 0.995f)
                {
                    return new Vector3(90.0f, 0.0f,
                        MathDefs.RadiansToDegrees((float) Math.Atan2(2.0f * (X * Z - W * Y),
                            1.0f - 2.0f * (Y * Y + Z * Z))));
                }
                else
                {
                    return new Vector3(
                        MathDefs.RadiansToDegrees((float) Math.Asin(check)),
                        MathDefs.RadiansToDegrees((float) Math.Atan2(2.0f * (X * Z + W * Y),
                            1.0f - 2.0f * (X * X + Y * Y))),
                        MathDefs.RadiansToDegrees((float) Math.Atan2(2.0f * (X * Y + W * Z),
                            1.0f - 2.0f * (X * X + Z * Z)))
                    );
                }
            }
        }

        /// Return yaw angle in degrees.
        public float YawAngle => EulerAngles.Y;

        /// Return pitch angle in degrees.
        public float PitchAngle => EulerAngles.X;

        /// Return roll angle in degrees.
        public float RollAngle => EulerAngles.Z;

        /// Return rotation axis.
        public Vector3 Axis => new Vector3(X, Y, Z) / (float) Math.Sqrt(1.0 - W * W);

        /// Return rotation angle.
        public float Angle => 2.0f * (float) Math.Acos(W);

        /// Return the rotation matrix that corresponds to this quaternion.
        public Matrix3 RotationMatrix => new Matrix3(
            1.0f - 2.0f * Y * Y - 2.0f * Z * Z,
            2.0f * X * Y - 2.0f * W * Z,
            2.0f * X * Z + 2.0f * W * Y,
            2.0f * X * Y + 2.0f * W * Z,
            1.0f - 2.0f * X * X - 2.0f * Z * Z,
            2.0f * Y * Z - 2.0f * W * X,
            2.0f * X * Z - 2.0f * W * Y,
            2.0f * Y * Z + 2.0f * W * X,
            1.0f - 2.0f * X * X - 2.0f * Y * Y
        );

        /// Spherical interpolation with another quaternion.
        public Quaternion Slerp(in Quaternion rhs, float t)
        {
            // Favor accuracy for native code builds
            float cosAngle = DotProduct(rhs);
            float sign = 1.0f;
            // Enable shortest path rotation
            if (cosAngle < 0.0f)
            {
                cosAngle = -cosAngle;
                sign = -1.0f;
            }

            float angle = (float) Math.Acos(cosAngle);
            float sinAngle = (float) Math.Sin(angle);
            float t1, t2;

            if (sinAngle > 0.001f)
            {
                float invSinAngle = 1.0f / sinAngle;
                t1 = (float) Math.Sin((1.0f - t) * angle) * invSinAngle;
                t2 = (float) Math.Sin(t * angle) * invSinAngle;
            }
            else
            {
                t1 = 1.0f - t;
                t2 = t;
            }

            return this * t1 + (rhs * sign) * t2;
        }

        /// Normalized linear interpolation with another quaternion.
        public Quaternion Nlerp(in Quaternion rhs, float t, bool shortestPath = false)
        {
            Quaternion result;
            float fCos = DotProduct(rhs);
            if (fCos < 0.0f && shortestPath)
                result = this + (-rhs - this) * t;
            else
                result = this + (rhs - this) * t;
            result.Normalize();
            return result;
        }

        /// Return float data.
        public float[] Data => new[] {W, X, Y, Z};

        /// Return as string.
        public override string ToString()
        {
            return $"{W} {X} {Y} {Z}";
        }

        public Vector3 Xyz => new Vector3(X, Y, Z);

        /// W coordinate.
        public float W;

        /// X coordinate.
        public float X;

        /// Y coordinate.
        public float Y;

        /// Z coordinate.
        public float Z;

        /// Identity quaternion.
        public static readonly Quaternion IDENTITY = new Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

        public override bool Equals(object obj)
        {
            return obj is Quaternion other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = W.GetHashCode();
                hashCode = (hashCode * 397) ^ X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                return hashCode;
            }
        }
    }
}
