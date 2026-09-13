// Copyright (c) 2023-2023 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

