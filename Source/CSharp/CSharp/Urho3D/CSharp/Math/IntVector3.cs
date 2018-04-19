/*
Copyright (c) 2006 - 2008 The Open Toolkit library.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

using System;
using System.Runtime.InteropServices;
using System.Xml.Serialization;

namespace Urho3D
{
    /// <summary>
    /// Represents a 3D vector using three single-precision floating-point numbers.
    /// </summary>
    /// <remarks>
    /// The IntVector3 structure is suitable for interoperation with unmanaged code requiring three consecutive floats.
    /// </remarks>
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct IntVector3 : IEquatable<IntVector3>
    {
        /// <summary>
        /// The X component of the IntVector3.
        /// </summary>
        public int X;

        /// <summary>
        /// The Y component of the IntVector3.
        /// </summary>
        public int Y;

        /// <summary>
        /// The Z component of the IntVector3.
        /// </summary>
        public int Z;

        /// <summary>
        /// Constructs a new instance.
        /// </summary>
        /// <param name="value">The value that will initialize this instance.</param>
        public IntVector3(int value)
        {
            X = value;
            Y = value;
            Z = value;
        }

        /// <summary>
        /// Constructs a new IntVector3.
        /// </summary>
        /// <param name="x">The x component of the IntVector3.</param>
        /// <param name="y">The y component of the IntVector3.</param>
        /// <param name="z">The z component of the IntVector3.</param>
        public IntVector3(int x, int y, int z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// <summary>
        /// Constructs a new IntVector3 from the given IntVector2.
        /// </summary>
        /// <param name="v">The IntVector2 to copy components from.</param>
        public IntVector3(IntVector2 v)
        {
            X = v.X;
            Y = v.Y;
            Z = 0;
        }

        /// <summary>
        /// Constructs a new IntVector3 from the given IntVector3.
        /// </summary>
        /// <param name="v">The IntVector3 to copy components from.</param>
        public IntVector3(IntVector3 v)
        {
            X = v.X;
            Y = v.Y;
            Z = v.Z;
        }

        /// <summary>
        /// Gets or sets the value at the index of the Vector.
        /// </summary>
        public int this[int index] {
            get{
                if (index == 0)
                {
                    return X;
                }
                else if (index == 1)
                {
                    return Y;
                }
                else if (index == 2)
                {
                    return Z;
                }
                throw new IndexOutOfRangeException("You tried to access this vector at index: " + index);
            } set{
                if (index == 0)
                {
                    X = value;
                }
                else if (index == 1)
                {
                    Y = value;
                }
                else if (index == 2)
                {
                    Z = value;
                }
                else
                {
                    throw new IndexOutOfRangeException("You tried to set this vector at index: " + index);
                }
            }
        }

        /// <summary>
        /// Gets the length (magnitude) of the vector.
        /// </summary>
        /// <see cref="LengthFast"/>
        /// <seealso cref="LengthSquared"/>
        public float Length
        {
            get
            {
                return (int)System.Math.Sqrt(X * X + Y * Y + Z * Z);
            }
        }

        /// <summary>
        /// Gets an approximation of the vector length (magnitude).
        /// </summary>
        /// <remarks>
        /// This property uses an approximation of the square root function to calculate vector magnitude, with
        /// an upper error bound of 0.001.
        /// </remarks>
        /// <see cref="Length"/>
        /// <seealso cref="LengthSquared"/>
        public float LengthFast
        {
            get
            {
                return 1.0f / MathDefs.InverseSqrtFast(X * X + Y * Y + Z * Z);
            }
        }

        /// <summary>
        /// Gets the square of the vector length (magnitude).
        /// </summary>
        /// <remarks>
        /// This property avoids the costly square root operation required by the Length property. This makes it more suitable
        /// for comparisons.
        /// </remarks>
        /// <see cref="Length"/>
        /// <seealso cref="LengthFast"/>
        public float LengthSquared
        {
            get
            {
                return X * X + Y * Y + Z * Z;
            }
        }

        /// <summary>
        /// Returns a copy of the IntVector3 scaled to unit length.
        /// </summary>
        public IntVector3 Normalized()
        {
            IntVector3 v = this;
            v.Normalize();
            return v;
        }

        /// <summary>
        /// Scales the IntVector3 to unit length.
        /// </summary>
        public void Normalize()
        {
            float scale = 1.0f / this.Length;
            X = (int)Math.Round(X * scale);
            Y = (int)Math.Round(Y * scale);
            Z = (int)Math.Round(Z * scale);
        }

        /// <summary>
        /// Scales the IntVector3 to approximately unit length.
        /// </summary>
        public void NormalizeFast()
        {
            float scale = MathDefs.InverseSqrtFast(X * X + Y * Y + Z * Z);
            X = (int)Math.Round(X * scale);
            Y = (int)Math.Round(Y * scale);
            Z = (int)Math.Round(Z * scale);
        }

        /// <summary>
        /// Defines a unit-length IntVector3 that points towards the X-axis.
        /// </summary>
        public static readonly IntVector3 UnitX = new IntVector3(1, 0, 0);

        /// <summary>
        /// Defines a unit-length IntVector3 that points towards the Y-axis.
        /// </summary>
        public static readonly IntVector3 UnitY = new IntVector3(0, 1, 0);

        /// <summary>
        /// Defines a unit-length IntVector3 that points towards the Z-axis.
        /// </summary>
        public static readonly IntVector3 UnitZ = new IntVector3(0, 0, 1);

        /// <summary>
        /// Defines a zero-length IntVector3.
        /// </summary>
        public static readonly IntVector3 Zero = new IntVector3(0, 0, 0);

        /// <summary>
        /// Defines an instance with all components set to 1.
        /// </summary>
        public static readonly IntVector3 One = new IntVector3(1, 1, 1);

        public static readonly IntVector3 Up = new IntVector3(0,  1, 0);
        public static readonly IntVector3 Down = new IntVector3(0, -1, 0);
        public static readonly IntVector3 Forward = new IntVector3(0, 0,  1);
        public static readonly IntVector3 Back = new IntVector3(0, 0, -1);
        public static readonly IntVector3 Left = new IntVector3(-1, 0, 0);
        public static readonly IntVector3 Right = new IntVector3( 1, 0, 0);

        /// <summary>
        /// Defines the size of the IntVector3 struct in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(new IntVector3());

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="a">Left operand.</param>
        /// <param name="b">Right operand.</param>
        /// <returns>Result of operation.</returns>
        public static IntVector3 Add(IntVector3 a, IntVector3 b)
        {
            Add(ref a, ref b, out a);
            return a;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="a">Left operand.</param>
        /// <param name="b">Right operand.</param>
        /// <param name="result">Result of operation.</param>
        public static void Add(ref IntVector3 a, ref IntVector3 b, out IntVector3 result)
        {
            result.X = a.X + b.X;
            result.Y = a.Y + b.Y;
            result.Z = a.Z + b.Z;
        }

        /// <summary>
        /// Subtract one Vector from another
        /// </summary>
        /// <param name="a">First operand</param>
        /// <param name="b">Second operand</param>
        /// <returns>Result of subtraction</returns>
        public static IntVector3 Subtract(IntVector3 a, IntVector3 b)
        {
            Subtract(ref a, ref b, out a);
            return a;
        }

        /// <summary>
        /// Subtract one Vector from another
        /// </summary>
        /// <param name="a">First operand</param>
        /// <param name="b">Second operand</param>
        /// <param name="result">Result of subtraction</param>
        public static void Subtract(ref IntVector3 a, ref IntVector3 b, out IntVector3 result)
        {
            result.X = a.X - b.X;
            result.Y = a.Y - b.Y;
            result.Z = a.Z - b.Z;
        }

        /// <summary>
        /// Multiplies a vector by a scalar.
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <returns>Result of the operation.</returns>
        public static IntVector3 Multiply(IntVector3 vector, int scale)
        {
            Multiply(ref vector, scale, out vector);
            return vector;
        }

        /// <summary>
        /// Multiplies a vector by a scalar.
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <param name="result">Result of the operation.</param>
        public static void Multiply(ref IntVector3 vector, int scale, out IntVector3 result)
        {
            result.X = vector.X * scale;
            result.Y = vector.Y * scale;
            result.Z = vector.Z * scale;
        }

        /// <summary>
        /// Multiplies a vector by the components a vector (scale).
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <returns>Result of the operation.</returns>
        public static IntVector3 Multiply(IntVector3 vector, IntVector3 scale)
        {
            Multiply(ref vector, ref scale, out vector);
            return vector;
        }

        /// <summary>
        /// Multiplies a vector by the components of a vector (scale).
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <param name="result">Result of the operation.</param>
        public static void Multiply(ref IntVector3 vector, ref IntVector3 scale, out IntVector3 result)
        {
            result.X = vector.X * scale.X;
            result.Y = vector.Y * scale.Y;
            result.Z = vector.Z * scale.Z;
        }

        /// <summary>
        /// Divides a vector by a scalar.
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <returns>Result of the operation.</returns>
        public static IntVector3 Divide(IntVector3 vector, int scale)
        {
            Divide(ref vector, scale, out vector);
            return vector;
        }

        /// <summary>
        /// Divides a vector by a scalar.
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <param name="result">Result of the operation.</param>
        public static void Divide(ref IntVector3 vector, int scale, out IntVector3 result)
        {
            result.X = vector.X / scale;
            result.Y = vector.Y / scale;
            result.Z = vector.Z / scale;
        }

        /// <summary>
        /// Divides a vector by the components of a vector (scale).
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <returns>Result of the operation.</returns>
        public static IntVector3 Divide(IntVector3 vector, IntVector3 scale)
        {
            Divide(ref vector, ref scale, out vector);
            return vector;
        }

        /// <summary>
        /// Divide a vector by the components of a vector (scale).
        /// </summary>
        /// <param name="vector">Left operand.</param>
        /// <param name="scale">Right operand.</param>
        /// <param name="result">Result of the operation.</param>
        public static void Divide(ref IntVector3 vector, ref IntVector3 scale, out IntVector3 result)
        {
            result.X = vector.X / scale.X;
            result.Y = vector.Y / scale.Y;
            result.Z = vector.Z / scale.Z;
        }

        /// <summary>
        /// Returns a vector created from the smallest of the corresponding components of the given vectors.
        /// </summary>
        /// <param name="a">First operand</param>
        /// <param name="b">Second operand</param>
        /// <returns>The component-wise minimum</returns>
        public static IntVector3 ComponentMin(IntVector3 a, IntVector3 b)
        {
            a.X = a.X < b.X ? a.X : b.X;
            a.Y = a.Y < b.Y ? a.Y : b.Y;
            a.Z = a.Z < b.Z ? a.Z : b.Z;
            return a;
        }

        /// <summary>
        /// Returns a vector created from the smallest of the corresponding components of the given vectors.
        /// </summary>
        /// <param name="a">First operand</param>
        /// <param name="b">Second operand</param>
        /// <param name="result">The component-wise minimum</param>
        public static void ComponentMin(ref IntVector3 a, ref IntVector3 b, out IntVector3 result)
        {
            result.X = a.X < b.X ? a.X : b.X;
            result.Y = a.Y < b.Y ? a.Y : b.Y;
            result.Z = a.Z < b.Z ? a.Z : b.Z;
        }

        /// <summary>
        /// Returns a vector created from the largest of the corresponding components of the given vectors.
        /// </summary>
        /// <param name="a">First operand</param>
        /// <param name="b">Second operand</param>
        /// <returns>The component-wise maximum</returns>
        public static IntVector3 ComponentMax(IntVector3 a, IntVector3 b)
        {
            a.X = a.X > b.X ? a.X : b.X;
            a.Y = a.Y > b.Y ? a.Y : b.Y;
            a.Z = a.Z > b.Z ? a.Z : b.Z;
            return a;
        }

        /// <summary>
        /// Returns a vector created from the largest of the corresponding components of the given vectors.
        /// </summary>
        /// <param name="a">First operand</param>
        /// <param name="b">Second operand</param>
        /// <param name="result">The component-wise maximum</param>
        public static void ComponentMax(ref IntVector3 a, ref IntVector3 b, out IntVector3 result)
        {
            result.X = a.X > b.X ? a.X : b.X;
            result.Y = a.Y > b.Y ? a.Y : b.Y;
            result.Z = a.Z > b.Z ? a.Z : b.Z;
        }

        /// <summary>
        /// Returns the IntVector3 with the minimum magnitude. If the magnitudes are equal, the second vector
        /// is selected.
        /// </summary>
        /// <param name="left">Left operand</param>
        /// <param name="right">Right operand</param>
        /// <returns>The minimum IntVector3</returns>
        public static IntVector3 MagnitudeMin(IntVector3 left, IntVector3 right)
        {
            return left.LengthSquared < right.LengthSquared ? left : right;
        }

        /// <summary>
        /// Returns the IntVector3 with the minimum magnitude. If the magnitudes are equal, the second vector
        /// is selected.
        /// </summary>
        /// <param name="left">Left operand</param>
        /// <param name="right">Right operand</param>
        /// <param name="result">The magnitude-wise minimum</param>
        /// <returns>The minimum IntVector3</returns>
        public static void MagnitudeMin(ref IntVector3 left, ref IntVector3 right, out IntVector3 result)
        {
            result = left.LengthSquared < right.LengthSquared ? left : right;
        }

        /// <summary>
        /// Returns the IntVector3 with the maximum magnitude. If the magnitudes are equal, the first vector
        /// is selected.
        /// </summary>
        /// <param name="left">Left operand</param>
        /// <param name="right">Right operand</param>
        /// <returns>The maximum IntVector3</returns>
        public static IntVector3 MagnitudeMax(IntVector3 left, IntVector3 right)
        {
            return left.LengthSquared >= right.LengthSquared ? left : right;
        }

        /// <summary>
        /// Returns the IntVector3 with the maximum magnitude. If the magnitudes are equal, the first vector
        /// is selected.
        /// </summary>
        /// <param name="left">Left operand</param>
        /// <param name="right">Right operand</param>
        /// <param name="result">The magnitude-wise maximum</param>
        /// <returns>The maximum IntVector3</returns>
        public static void MagnitudeMax(ref IntVector3 left, ref IntVector3 right, out IntVector3 result)
        {
            result = left.LengthSquared >= right.LengthSquared ? left : right;
        }

        /// <summary>
        /// Returns the IntVector3 with the minimum magnitude
        /// </summary>
        /// <param name="left">Left operand</param>
        /// <param name="right">Right operand</param>
        /// <returns>The minimum IntVector3</returns>
        [Obsolete("Use MagnitudeMin() instead.")]
        public static IntVector3 Min(IntVector3 left, IntVector3 right)
        {
            return left.LengthSquared < right.LengthSquared ? left : right;
        }

        /// <summary>
        /// Returns the IntVector3 with the minimum magnitude
        /// </summary>
        /// <param name="left">Left operand</param>
        /// <param name="right">Right operand</param>
        /// <returns>The minimum IntVector3</returns>
        [Obsolete("Use MagnitudeMax() instead.")]
        public static IntVector3 Max(IntVector3 left, IntVector3 right)
        {
            return left.LengthSquared >= right.LengthSquared ? left : right;
        }

        /// <summary>
        /// Clamp a vector to the given minimum and maximum vectors
        /// </summary>
        /// <param name="vec">Input vector</param>
        /// <param name="min">Minimum vector</param>
        /// <param name="max">Maximum vector</param>
        /// <returns>The clamped vector</returns>
        public static IntVector3 Clamp(IntVector3 vec, IntVector3 min, IntVector3 max)
        {
            vec.X = vec.X < min.X ? min.X : vec.X > max.X ? max.X : vec.X;
            vec.Y = vec.Y < min.Y ? min.Y : vec.Y > max.Y ? max.Y : vec.Y;
            vec.Z = vec.Z < min.Z ? min.Z : vec.Z > max.Z ? max.Z : vec.Z;
            return vec;
        }

        /// <summary>
        /// Clamp a vector to the given minimum and maximum vectors
        /// </summary>
        /// <param name="vec">Input vector</param>
        /// <param name="min">Minimum vector</param>
        /// <param name="max">Maximum vector</param>
        /// <param name="result">The clamped vector</param>
        public static void Clamp(ref IntVector3 vec, ref IntVector3 min, ref IntVector3 max, out IntVector3 result)
        {
            result.X = vec.X < min.X ? min.X : vec.X > max.X ? max.X : vec.X;
            result.Y = vec.Y < min.Y ? min.Y : vec.Y > max.Y ? max.Y : vec.Y;
            result.Z = vec.Z < min.Z ? min.Z : vec.Z > max.Z ? max.Z : vec.Z;
        }

        /// <summary>
        /// Compute the euclidean distance between two vectors.
        /// </summary>
        /// <param name="vec1">The first vector</param>
        /// <param name="vec2">The second vector</param>
        /// <returns>The distance</returns>
        public static int Distance(IntVector3 vec1, IntVector3 vec2)
        {
            int result;
            Distance(ref vec1, ref vec2, out result);
            return result;
        }

        /// <summary>
        /// Compute the euclidean distance between two vectors.
        /// </summary>
        /// <param name="vec1">The first vector</param>
        /// <param name="vec2">The second vector</param>
        /// <param name="result">The distance</param>
        public static void Distance(ref IntVector3 vec1, ref IntVector3 vec2, out int result)
        {
            result = (int)Math.Sqrt((vec2.X - vec1.X) * (vec2.X - vec1.X) + (vec2.Y - vec1.Y) * (vec2.Y - vec1.Y) + (vec2.Z - vec1.Z) * (vec2.Z - vec1.Z));
        }

        /// <summary>
        /// Compute the squared euclidean distance between two vectors.
        /// </summary>
        /// <param name="vec1">The first vector</param>
        /// <param name="vec2">The second vector</param>
        /// <returns>The squared distance</returns>
        public static int DistanceSquared(IntVector3 vec1, IntVector3 vec2)
        {
            int result;
            DistanceSquared(ref vec1, ref vec2, out result);
            return result;
        }

        /// <summary>
        /// Compute the squared euclidean distance between two vectors.
        /// </summary>
        /// <param name="vec1">The first vector</param>
        /// <param name="vec2">The second vector</param>
        /// <param name="result">The squared distance</param>
        public static void DistanceSquared(ref IntVector3 vec1, ref IntVector3 vec2, out int result)
        {
            result = (vec2.X - vec1.X) * (vec2.X - vec1.X) + (vec2.Y - vec1.Y) * (vec2.Y - vec1.Y) + (vec2.Z - vec1.Z) * (vec2.Z - vec1.Z);
        }

        /// <summary>
        /// Scale a vector to unit length
        /// </summary>
        /// <param name="vec">The input vector</param>
        /// <returns>The normalized vector</returns>
        public static IntVector3 Normalize(IntVector3 vec)
        {
            float scale = 1.0f / vec.Length;
            vec.X = (int)Math.Round(vec.X * scale);
            vec.Y = (int)Math.Round(vec.Y * scale);
            vec.Z = (int)Math.Round(vec.Z * scale);
            return vec;
        }

        /// <summary>
        /// Scale a vector to unit length
        /// </summary>
        /// <param name="vec">The input vector</param>
        /// <param name="result">The normalized vector</param>
        public static void Normalize(ref IntVector3 vec, out IntVector3 result)
        {
            float scale = 1.0f / vec.Length;
            result.X = (int)Math.Round(vec.X * scale);
            result.Y = (int)Math.Round(vec.Y * scale);
            result.Z = (int)Math.Round(vec.Z * scale);
        }

        /// <summary>
        /// Scale a vector to approximately unit length
        /// </summary>
        /// <param name="vec">The input vector</param>
        /// <returns>The normalized vector</returns>
        public static IntVector3 NormalizeFast(IntVector3 vec)
        {
            float scale = MathDefs.InverseSqrtFast(vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z);
            vec.X = (int)Math.Round(vec.X * scale);
            vec.Y = (int)Math.Round(vec.Y * scale);
            vec.Z = (int)Math.Round(vec.Z * scale);
            return vec;
        }

        /// <summary>
        /// Scale a vector to approximately unit length
        /// </summary>
        /// <param name="vec">The input vector</param>
        /// <param name="result">The normalized vector</param>
        public static void NormalizeFast(ref IntVector3 vec, out IntVector3 result)
        {
            float scale = MathDefs.InverseSqrtFast(vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z);
            result.X = (int)Math.Round(vec.X * scale);
            result.Y = (int)Math.Round(vec.Y * scale);
            result.Z = (int)Math.Round(vec.Z * scale);
        }

        /// <summary>
        /// Calculate the dot (scalar) product of two vectors
        /// </summary>
        /// <param name="left">First operand</param>
        /// <param name="right">Second operand</param>
        /// <returns>The dot product of the two inputs</returns>
        public static int Dot(IntVector3 left, IntVector3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculate the dot (scalar) product of two vectors
        /// </summary>
        /// <param name="left">First operand</param>
        /// <param name="right">Second operand</param>
        /// <param name="result">The dot product of the two inputs</param>
        public static void Dot(ref IntVector3 left, ref IntVector3 right, out int result)
        {
            result = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Caclulate the cross (vector) product of two vectors
        /// </summary>
        /// <param name="left">First operand</param>
        /// <param name="right">Second operand</param>
        /// <returns>The cross product of the two inputs</returns>
        public static IntVector3 Cross(IntVector3 left, IntVector3 right)
        {
            IntVector3 result;
            Cross(ref left, ref right, out result);
            return result;
        }

        /// <summary>
        /// Caclulate the cross (vector) product of two vectors
        /// </summary>
        /// <remarks>
        /// It is incorrect to call this method passing the same variable for
        /// <paramref name="result"/> as for <paramref name="left"/> or
        /// <paramref name="right"/>.
        /// </remarks>
        /// <param name="left">First operand</param>
        /// <param name="right">Second operand</param>
        /// <returns>The cross product of the two inputs</returns>
        /// <param name="result">The cross product of the two inputs</param>
        public static void Cross(ref IntVector3 left, ref IntVector3 right, out IntVector3 result)
        {
            result.X = left.Y * right.Z - left.Z * right.Y;
            result.Y = left.Z * right.X - left.X * right.Z;
            result.Z = left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Returns a new Vector that is the linear blend of the 2 given Vectors
        /// </summary>
        /// <param name="a">First input vector</param>
        /// <param name="b">Second input vector</param>
        /// <param name="blend">The blend factor. a when blend=0, b when blend=1.</param>
        /// <returns>a when blend=0, b when blend=1, and a linear combination otherwise</returns>
        public static IntVector3 Lerp(IntVector3 a, IntVector3 b, int blend)
        {
            a.X = blend * (b.X - a.X) + a.X;
            a.Y = blend * (b.Y - a.Y) + a.Y;
            a.Z = blend * (b.Z - a.Z) + a.Z;
            return a;
        }

        /// <summary>
        /// Returns a new Vector that is the linear blend of the 2 given Vectors
        /// </summary>
        /// <param name="a">First input vector</param>
        /// <param name="b">Second input vector</param>
        /// <param name="blend">The blend factor. a when blend=0, b when blend=1.</param>
        /// <param name="result">a when blend=0, b when blend=1, and a linear combination otherwise</param>
        public static void Lerp(ref IntVector3 a, ref IntVector3 b, int blend, out IntVector3 result)
        {
            result.X = blend * (b.X - a.X) + a.X;
            result.Y = blend * (b.Y - a.Y) + a.Y;
            result.Z = blend * (b.Z - a.Z) + a.Z;
        }

        /// <summary>
        /// Interpolate 3 Vectors using Barycentric coordinates
        /// </summary>
        /// <param name="a">First input Vector</param>
        /// <param name="b">Second input Vector</param>
        /// <param name="c">Third input Vector</param>
        /// <param name="u">First Barycentric Coordinate</param>
        /// <param name="v">Second Barycentric Coordinate</param>
        /// <returns>a when u=v=0, b when u=1,v=0, c when u=0,v=1, and a linear combination of a,b,c otherwise</returns>
        public static IntVector3 BaryCentric(IntVector3 a, IntVector3 b, IntVector3 c, int u, int v)
        {
            return a + u * (b - a) + v * (c - a);
        }

        /// <summary>Interpolate 3 Vectors using Barycentric coordinates</summary>
        /// <param name="a">First input Vector.</param>
        /// <param name="b">Second input Vector.</param>
        /// <param name="c">Third input Vector.</param>
        /// <param name="u">First Barycentric Coordinate.</param>
        /// <param name="v">Second Barycentric Coordinate.</param>
        /// <param name="result">Output Vector. a when u=v=0, b when u=1,v=0, c when u=0,v=1, and a linear combination of a,b,c otherwise</param>
        public static void BaryCentric(ref IntVector3 a, ref IntVector3 b, ref IntVector3 c, int u, int v, out IntVector3 result)
        {
            result = a; // copy

            IntVector3 temp = b; // copy
            Subtract(ref temp, ref a, out temp);
            Multiply(ref temp, u, out temp);
            Add(ref result, ref temp, out result);

            temp = c; // copy
            Subtract(ref temp, ref a, out temp);
            Multiply(ref temp, v, out temp);
            Add(ref result, ref temp, out result);
        }

        /// <summary>Transform a direction vector by the given Matrix
        /// Assumes the matrix has a bottom row of (0,0,0,1), that is the translation part is ignored.
        /// </summary>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <returns>The transformed vector</returns>
        public static IntVector3 TransformVector(IntVector3 vec, Matrix4 mat)
        {
            IntVector3 result;
            TransformVector(ref vec, ref mat, out result);
            return result;
        }

        /// <summary>Transform a direction vector by the given Matrix
        /// Assumes the matrix has a bottom row of (0,0,0,1), that is the translation part is ignored.
        /// </summary>
        /// <remarks>
        /// It is incorrect to call this method passing the same variable for
        /// <paramref name="result"/> as for <paramref name="vec"/>.
        /// </remarks>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <param name="result">The transformed vector</param>
        public static void TransformVector(ref IntVector3 vec, ref Matrix4 mat, out IntVector3 result)
        {
            result.X = (int)Math.Round(vec.X * mat.Row0.X +
                       vec.Y * mat.Row1.X +
                       vec.Z * mat.Row2.X);

            result.Y = (int)Math.Round(vec.X * mat.Row0.Y +
                       vec.Y * mat.Row1.Y +
                       vec.Z * mat.Row2.Y);

            result.Z = (int)Math.Round(vec.X * mat.Row0.Z +
                       vec.Y * mat.Row1.Z +
                       vec.Z * mat.Row2.Z);
        }

        /// <summary>Transform a Normal by the given Matrix</summary>
        /// <remarks>
        /// This calculates the inverse of the given matrix, use TransformNormalInverse if you
        /// already have the inverse to avoid this extra calculation
        /// </remarks>
        /// <param name="norm">The normal to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <returns>The transformed normal</returns>
        public static IntVector3 TransformNormal(IntVector3 norm, Matrix4 mat)
        {
            IntVector3 result;
            TransformNormal(ref norm, ref mat, out result);
            return result;
        }

        /// <summary>Transform a Normal by the given Matrix</summary>
        /// <remarks>
        /// This calculates the inverse of the given matrix, use TransformNormalInverse if you
        /// already have the inverse to avoid this extra calculation
        /// </remarks>
        /// <param name="norm">The normal to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <param name="result">The transformed normal</param>
        public static void TransformNormal(ref IntVector3 norm, ref Matrix4 mat, out IntVector3 result)
        {
            Matrix4 Inverse = Matrix4.Invert(mat);
            IntVector3.TransformNormalInverse(ref norm, ref Inverse, out result);
        }

        /// <summary>Transform a Normal by the (transpose of the) given Matrix</summary>
        /// <remarks>
        /// This version doesn't calculate the inverse matrix.
        /// Use this version if you already have the inverse of the desired transform to hand
        /// </remarks>
        /// <param name="norm">The normal to transform</param>
        /// <param name="invMat">The inverse of the desired transformation</param>
        /// <returns>The transformed normal</returns>
        public static IntVector3 TransformNormalInverse(IntVector3 norm, Matrix4 invMat)
        {
            IntVector3 result;
            TransformNormalInverse(ref norm, ref invMat, out result);
            return result;
        }

        /// <summary>Transform a Normal by the (transpose of the) given Matrix</summary>
        /// <remarks>
        /// This version doesn't calculate the inverse matrix.
        /// Use this version if you already have the inverse of the desired transform to hand
        /// </remarks>
        /// <param name="norm">The normal to transform</param>
        /// <param name="invMat">The inverse of the desired transformation</param>
        /// <param name="result">The transformed normal</param>
        public static void TransformNormalInverse(ref IntVector3 norm, ref Matrix4 invMat, out IntVector3 result)
        {
            result.X = (int)Math.Round(norm.X * invMat.Row0.X +
                       norm.Y * invMat.Row0.Y +
                       norm.Z * invMat.Row0.Z);

            result.Y = (int)Math.Round(norm.X * invMat.Row1.X +
                       norm.Y * invMat.Row1.Y +
                       norm.Z * invMat.Row1.Z);

            result.Z = (int)Math.Round(norm.X * invMat.Row2.X +
                       norm.Y * invMat.Row2.Y +
                       norm.Z * invMat.Row2.Z);
        }

        /// <summary>Transform a Position by the given Matrix</summary>
        /// <param name="pos">The position to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <returns>The transformed position</returns>
        public static IntVector3 TransformPosition(IntVector3 pos, Matrix4 mat)
        {
            IntVector3 result;
            TransformPosition(ref pos, ref mat, out result);
            return result;
        }

        /// <summary>Transform a Position by the given Matrix</summary>
        /// <param name="pos">The position to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <param name="result">The transformed position</param>
        public static void TransformPosition(ref IntVector3 pos, ref Matrix4 mat, out IntVector3 result)
        {
            result.X = (int)Math.Round(pos.X * mat.Row0.X +
                       pos.Y * mat.Row1.X +
                       pos.Z * mat.Row2.X +
                       mat.Row3.X);

            result.Y = (int)Math.Round(pos.X * mat.Row0.Y +
                       pos.Y * mat.Row1.Y +
                       pos.Z * mat.Row2.Y +
                       mat.Row3.Y);

            result.Z = (int)Math.Round(pos.X * mat.Row0.Z +
                       pos.Y * mat.Row1.Z +
                       pos.Z * mat.Row2.Z +
                       mat.Row3.Z);
        }

        /// <summary>Transform a Vector by the given Matrix</summary>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <returns>The transformed vector</returns>
        public static IntVector3 Transform(IntVector3 vec, Matrix3 mat)
        {
            IntVector3 result;
            Transform(ref vec, ref mat, out result);
            return result;
        }

        /// <summary>Transform a Vector by the given Matrix</summary>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <param name="result">The transformed vector</param>
        public static void Transform(ref IntVector3 vec, ref Matrix3 mat, out IntVector3 result)
        {
            result.X = (int)Math.Round(vec.X * mat.Row0.X + vec.Y * mat.Row1.X + vec.Z * mat.Row2.X);
            result.Y = (int)Math.Round(vec.X * mat.Row0.Y + vec.Y * mat.Row1.Y + vec.Z * mat.Row2.Y);
            result.Z = (int)Math.Round(vec.X * mat.Row0.Z + vec.Y * mat.Row1.Z + vec.Z * mat.Row2.Z);
        }

        /// <summary>
        /// Transforms a vector by a quaternion rotation.
        /// </summary>
        /// <param name="vec">The vector to transform.</param>
        /// <param name="quat">The quaternion to rotate the vector by.</param>
        /// <returns>The result of the operation.</returns>
        public static IntVector3 Transform(IntVector3 vec, Quaternion quat)
        {
            IntVector3 result;
            Transform(ref vec, ref quat, out result);
            return result;
        }

        /// <summary>
        /// Transforms a vector by a quaternion rotation.
        /// </summary>
        /// <param name="vec">The vector to transform.</param>
        /// <param name="quat">The quaternion to rotate the vector by.</param>
        /// <param name="result">The result of the operation.</param>
        public static void Transform(ref IntVector3 vec, ref Quaternion quat, out IntVector3 result)
        {
            // Since vec.W == 0, we can optimize quat * vec * quat^-1 as follows:
            // vec + 2.0 * cross(quat.xyz, cross(quat.xyz, vec) + quat.w * vec)
            Vector3 qxyz = quat.Xyz;
            IntVector3 xyz = new IntVector3((int)qxyz.X, (int)qxyz.Y, (int)qxyz.Z), temp, temp2;
            IntVector3.Cross(ref xyz, ref vec, out temp);
            IntVector3.Multiply(ref vec, (int)quat.W, out temp2);
            IntVector3.Add(ref temp, ref temp2, out temp);
            IntVector3.Cross(ref xyz, ref temp, out temp);
            IntVector3.Multiply(ref temp, 2, out temp);
            IntVector3.Add(ref vec, ref temp, out result);
        }

        /// <summary>Transform a Vector by the given Matrix using right-handed notation</summary>
        /// <param name="mat">The desired transformation</param>
        /// <param name="vec">The vector to transform</param>
        public static IntVector3 Transform(Matrix3 mat, IntVector3 vec)
        {
            IntVector3 result;
            Transform(ref vec, ref mat, out result);
            return result;
        }

        /// <summary>Transform a Vector by the given Matrix using right-handed notation</summary>
        /// <param name="mat">The desired transformation</param>
        /// <param name="vec">The vector to transform</param>
        /// <param name="result">The transformed vector</param>
        public static void Transform(ref Matrix3 mat, ref IntVector3 vec, out IntVector3 result)
        {
            result.X = (int)Math.Round(mat.Row0.X * vec.X + mat.Row0.Y * vec.Y + mat.Row0.Z * vec.Z);
            result.Y = (int)Math.Round(mat.Row1.X * vec.X + mat.Row1.Y * vec.Y + mat.Row1.Z * vec.Z);
            result.Z = (int)Math.Round(mat.Row2.X * vec.X + mat.Row2.Y * vec.Y + mat.Row2.Z * vec.Z);
        }

        /// <summary>Transform a IntVector3 by the given Matrix, and project the resulting Vector4 back to a IntVector3</summary>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <returns>The transformed vector</returns>
        public static IntVector3 TransformPerspective(IntVector3 vec, Matrix4 mat)
        {
            IntVector3 result;
            TransformPerspective(ref vec, ref mat, out result);
            return result;
        }

        /// <summary>Transform a IntVector3 by the given Matrix, and project the resulting Vector4 back to a IntVector3</summary>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <param name="result">The transformed vector</param>
        public static void TransformPerspective(ref IntVector3 vec, ref Matrix4 mat, out IntVector3 result)
        {
            Vector4 v = new Vector4(vec.X, vec.Y, vec.Z, 1);
            Vector4.Transform(ref v, ref mat, out v);
            result.X = (int)Math.Round(v.X / v.W);
            result.Y = (int)Math.Round(v.Y / v.W);
            result.Z = (int)Math.Round(v.Z / v.W);
        }

        /// <summary>
        /// Calculates the angle (in radians) between two vectors.
        /// </summary>
        /// <param name="first">The first vector.</param>
        /// <param name="second">The second vector.</param>
        /// <returns>Angle (in radians) between the vectors.</returns>
        /// <remarks>Note that the returned angle is never bigger than the constant Pi.</remarks>
        public static int CalculateAngle(IntVector3 first, IntVector3 second)
        {
            int result;
            CalculateAngle(ref first, ref second, out result);
            return result;
        }

        /// <summary>Calculates the angle (in radians) between two vectors.</summary>
        /// <param name="first">The first vector.</param>
        /// <param name="second">The second vector.</param>
        /// <param name="result">Angle (in radians) between the vectors.</param>
        /// <remarks>Note that the returned angle is never bigger than the constant Pi.</remarks>
        public static void CalculateAngle(ref IntVector3 first, ref IntVector3 second, out int result)
        {
            int temp;
            IntVector3.Dot(ref first, ref second, out temp);
            result = (int)System.Math.Acos(MathDefs.Clamp(temp / (first.Length * second.Length), -1.0, 1.0));
        }

        /// <summary>
        /// Projects a vector from object space into screen space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X coordinate of the viewport.</param>
        /// <param name="y">The Y coordinate of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The world-view-projection matrix.</param>
        /// <returns>The vector in screen space.</returns>
        /// <remarks>
        /// To project to normalized device coordinates (NDC) use the following parameters:
        /// Project(vector, -1, -1, 2, 2, -1, 1, worldViewProjection).
        /// </remarks>
        public static IntVector3 Project(IntVector3 vector, int x, int y, int width, int height, int minZ, int maxZ, Matrix4 worldViewProjection)
        {
            Vector4 result;

            result.X =
                vector.X * worldViewProjection.M00 +
                vector.Y * worldViewProjection.M10 +
                vector.Z * worldViewProjection.M20 +
                worldViewProjection.M30;

            result.Y =
                vector.X * worldViewProjection.M01 +
                vector.Y * worldViewProjection.M11 +
                vector.Z * worldViewProjection.M21 +
                worldViewProjection.M31;

            result.Z =
                vector.X * worldViewProjection.M02 +
                vector.Y * worldViewProjection.M12 +
                vector.Z * worldViewProjection.M22 +
                worldViewProjection.M32;

            result.W =
                vector.X * worldViewProjection.M03 +
                vector.Y * worldViewProjection.M13 +
                vector.Z * worldViewProjection.M23 +
                worldViewProjection.M33;

            result /= result.W;

            result.X = x + (width * ((result.X + 1.0f) / 2.0f));
            result.Y = y + (height * ((result.Y + 1.0f) / 2.0f));
            result.Z = minZ + ((maxZ - minZ) * ((result.Z + 1.0f) / 2.0f));

            return new IntVector3((int)result.X, (int)result.Y, (int)result.Z);
        }

        /// <summary>
        /// Projects a vector from screen space into object space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X coordinate of the viewport.</param>
        /// <param name="y">The Y coordinate of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="inverseWorldViewProjection">The inverse of the world-view-projection matrix.</param>
        /// <returns>The vector in object space.</returns>
        /// <remarks>
        /// To project from normalized device coordinates (NDC) use the following parameters:
        /// Project(vector, -1, -1, 2, 2, -1, 1, inverseWorldViewProjection).
        /// </remarks>
        public static IntVector3 Unproject(IntVector3 vector, int x, int y, int width, int height, int minZ, int maxZ, Matrix4 inverseWorldViewProjection)
        {
            Vector4 result;

            result.X = ((((vector.X - x) / width) * 2.0f) - 1.0f);
            result.Y = ((((vector.Y - y) / height) * 2.0f) - 1.0f);
            result.Z = (((vector.Z / (maxZ - minZ)) * 2.0f) - 1.0f);

            result.X =
                result.X * inverseWorldViewProjection.M00 +
                result.Y * inverseWorldViewProjection.M10 +
                result.Z * inverseWorldViewProjection.M20 +
                inverseWorldViewProjection.M30;

            result.Y =
                result.X * inverseWorldViewProjection.M01 +
                result.Y * inverseWorldViewProjection.M11 +
                result.Z * inverseWorldViewProjection.M21 +
                inverseWorldViewProjection.M31;

            result.Z =
                result.X * inverseWorldViewProjection.M02 +
                result.Y * inverseWorldViewProjection.M12 +
                result.Z * inverseWorldViewProjection.M22 +
                inverseWorldViewProjection.M32;

            result.W =
                result.X * inverseWorldViewProjection.M03 +
                result.Y * inverseWorldViewProjection.M13 +
                result.Z * inverseWorldViewProjection.M23 +
                inverseWorldViewProjection.M33;

            result /= result.W;

            return new IntVector3((int)result.X, (int)result.Y, (int)result.Z);
        }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector2 with the X and Y components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector2 Xy { get { return new IntVector2(X, Y); } set { X = value.X; Y = value.Y; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector2 with the X and Z components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector2 Xz { get { return new IntVector2(X, Z); } set { X = value.X; Z = value.Y; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector2 with the Y and X components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector2 Yx { get { return new IntVector2(Y, X); } set { Y = value.X; X = value.Y; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector2 with the Y and Z components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector2 Yz { get { return new IntVector2(Y, Z); } set { Y = value.X; Z = value.Y; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector2 with the Z and X components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector2 Zx { get { return new IntVector2(Z, X); } set { Z = value.X; X = value.Y; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector2 with the Z and Y components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector2 Zy { get { return new IntVector2(Z, Y); } set { Z = value.X; Y = value.Y; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector3 with the X, Z, and Y components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector3 Xzy { get { return new IntVector3(X, Z, Y); } set { X = value.X; Z = value.Y; Y = value.Z; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector3 with the Y, X, and Z components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector3 Yxz { get { return new IntVector3(Y, X, Z); } set { Y = value.X; X = value.Y; Z = value.Z; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector3 with the Y, Z, and X components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector3 Yzx { get { return new IntVector3(Y, Z, X); } set { Y = value.X; Z = value.Y; X = value.Z; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector3 with the Z, X, and Y components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector3 Zxy { get { return new IntVector3(Z, X, Y); } set { Z = value.X; X = value.Y; Y = value.Z; } }

        /// <summary>
        /// Gets or sets an OpenTK.IntVector3 with the Z, Y, and X components of this instance.
        /// </summary>
        [XmlIgnore]
        public IntVector3 Zyx { get { return new IntVector3(Z, Y, X); } set { Z = value.X; Y = value.Y; X = value.Z; } }

        /// <summary>
        /// Adds two instances.
        /// </summary>
        /// <param name="left">The first instance.</param>
        /// <param name="right">The second instance.</param>
        /// <returns>The result of the calculation.</returns>
        public static IntVector3 operator +(IntVector3 left, IntVector3 right)
        {
            left.X += right.X;
            left.Y += right.Y;
            left.Z += right.Z;
            return left;
        }

        /// <summary>
        /// Subtracts two instances.
        /// </summary>
        /// <param name="left">The first instance.</param>
        /// <param name="right">The second instance.</param>
        /// <returns>The result of the calculation.</returns>
        public static IntVector3 operator -(IntVector3 left, IntVector3 right)
        {
            left.X -= right.X;
            left.Y -= right.Y;
            left.Z -= right.Z;
            return left;
        }

        /// <summary>
        /// Negates an instance.
        /// </summary>
        /// <param name="vec">The instance.</param>
        /// <returns>The result of the calculation.</returns>
        public static IntVector3 operator -(IntVector3 vec)
        {
            vec.X = -vec.X;
            vec.Y = -vec.Y;
            vec.Z = -vec.Z;
            return vec;
        }

        /// <summary>
        /// Multiplies an instance by a scalar.
        /// </summary>
        /// <param name="vec">The instance.</param>
        /// <param name="scale">The scalar.</param>
        /// <returns>The result of the calculation.</returns>
        public static IntVector3 operator *(IntVector3 vec, int scale)
        {
            vec.X *= scale;
            vec.Y *= scale;
            vec.Z *= scale;
            return vec;
        }

        /// <summary>
        /// Multiplies an instance by a scalar.
        /// </summary>
        /// <param name="scale">The scalar.</param>
        /// <param name="vec">The instance.</param>
        /// <returns>The result of the calculation.</returns>
        public static IntVector3 operator *(int scale, IntVector3 vec)
        {
            vec.X *= scale;
            vec.Y *= scale;
            vec.Z *= scale;
            return vec;
        }

        /// <summary>
        /// Component-wise multiplication between the specified instance by a scale vector.
        /// </summary>
        /// <param name="scale">Left operand.</param>
        /// <param name="vec">Right operand.</param>
        /// <returns>Result of multiplication.</returns>
        public static IntVector3 operator *(IntVector3 vec, IntVector3 scale)
        {
            vec.X *= scale.X;
            vec.Y *= scale.Y;
            vec.Z *= scale.Z;
            return vec;
        }

        /// <summary>
        /// Transform a Vector by the given Matrix.
        /// </summary>
        /// <param name="vec">The vector to transform</param>
        /// <param name="mat">The desired transformation</param>
        /// <returns>The transformed vector</returns>
        public static IntVector3 operator *(IntVector3 vec, Matrix3 mat)
        {
            IntVector3 result;
            IntVector3.Transform(ref vec, ref mat, out result);
            return result;
        }

        /// <summary>
        /// Transform a Vector by the given Matrix using right-handed notation
        /// </summary>
        /// <param name="mat">The desired transformation</param>
        /// <param name="vec">The vector to transform</param>
        /// <returns>The transformed vector</returns>
        public static IntVector3 operator *(Matrix3 mat, IntVector3 vec)
        {
            IntVector3 result;
            IntVector3.Transform(ref mat, ref vec, out result);
            return result;
        }

        /// <summary>
        /// Transforms a vector by a quaternion rotation.
        /// </summary>
        /// <param name="vec">The vector to transform.</param>
        /// <param name="quat">The quaternion to rotate the vector by.</param>
        /// <returns></returns>
        public static IntVector3 operator *(Quaternion quat, IntVector3 vec)
        {
            IntVector3 result;
            IntVector3.Transform(ref vec, ref quat, out result);
            return result;
        }

        /// <summary>
        /// Divides an instance by a scalar.
        /// </summary>
        /// <param name="vec">The instance.</param>
        /// <param name="scale">The scalar.</param>
        /// <returns>The result of the calculation.</returns>
        public static IntVector3 operator /(IntVector3 vec, int scale)
        {
            vec.X /= scale;
            vec.Y /= scale;
            vec.Z /= scale;
            return vec;
        }

        /// <summary>
        /// Compares two instances for equality.
        /// </summary>
        /// <param name="left">The first instance.</param>
        /// <param name="right">The second instance.</param>
        /// <returns>True, if left equals right; false otherwise.</returns>
        public static bool operator ==(IntVector3 left, IntVector3 right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compares two instances for inequality.
        /// </summary>
        /// <param name="left">The first instance.</param>
        /// <param name="right">The second instance.</param>
        /// <returns>True, if left does not equal right; false otherwise.</returns>
        public static bool operator !=(IntVector3 left, IntVector3 right)
        {
            return !left.Equals(right);
        }

        private static string listSeparator = System.Globalization.CultureInfo.CurrentCulture.TextInfo.ListSeparator;
        /// <summary>
        /// Returns a System.String that represents the current IntVector3.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return String.Format("({0}{3} {1}{3} {2})", X, Y, Z, listSeparator);
        }

        /// <summary>
        /// Returns the hashcode for this instance.
        /// </summary>
        /// <returns>A System.Int32 containing the unique hashcode for this instance.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = this.X.GetHashCode();
                hashCode = (hashCode * 397) ^ this.Y.GetHashCode();
                hashCode = (hashCode * 397) ^ this.Z.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Indicates whether this instance and a specified object are equal.
        /// </summary>
        /// <param name="obj">The object to compare to.</param>
        /// <returns>True if the instances are equal; false otherwise.</returns>
        public override bool Equals(object obj)
        {
            if (!(obj is IntVector3))
            {
                return false;
            }

            return this.Equals((IntVector3)obj);
        }

        /// <summary>Indicates whether the current vector is equal to another vector.</summary>
        /// <param name="other">A vector to compare with this vector.</param>
        /// <returns>true if the current vector is equal to the vector parameter; otherwise, false.</returns>
        public bool Equals(IntVector3 other)
        {
            return
                X == other.X &&
                Y == other.Y &&
                Z == other.Z;
        }
    }
}
