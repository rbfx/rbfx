//
// Copyright (c) 2023-2023 the Urho3D project.
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

using System.Collections.Generic;

namespace Urho3DNet
{
    public class ApproximateEqualityComparer<T> : IEqualityComparer<T> where T : IApproximateEquatable<T>
    {
        public static readonly ApproximateEqualityComparer<T> Default = new ApproximateEqualityComparer<T>();

        private readonly float _epsilon;

        public ApproximateEqualityComparer(float epsilon = MathDefs.Epsilon)
        {
            _epsilon = epsilon;
        }

        /// <summary>When overridden in a derived class, determines whether two objects of type <typeparamref name="T" /> are equal.</summary>
        /// <param name="x">The first object to compare.</param>
        /// <param name="y">The second object to compare.</param>
        /// <returns>
        /// <see langword="true" /> if the specified objects are equal; otherwise, <see langword="false" />.</returns>
        public bool Equals(T x, T y)
        {
            return x.ApproximatelyEquivalent(y, _epsilon);
        }

        /// <summary>When overridden in a derived class, serves as a hash function for the specified object for hashing algorithms and data structures, such as a hash table.</summary>
        /// <param name="obj">The object for which to get a hash code.</param>
        /// <returns>A hash code for the specified object.</returns>
        /// <exception cref="T:System.ArgumentNullException">The type of <paramref name="obj" /> is a reference type and <paramref name="obj" /> is <see langword="null" />.</exception>
        public int GetHashCode(T obj)
        {
            return obj.GetHashCode();
        }
    }
}

