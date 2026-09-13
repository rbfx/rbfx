// Copyright (c) 2023-2023 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Collections.Generic;

namespace Urho3DNet
{
    public interface IApproximateEquatable<in T>
    {
        /// <summary>
        /// Test for equality with another object with epsilon.
        /// </summary>
        bool ApproximatelyEquivalent(T rhs, float epsilon = MathDefs.Epsilon);
    };
}
