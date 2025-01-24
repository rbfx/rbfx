// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    /// <summary>
    /// Add interface to the list of base types to allow <see cref="Node.GetDerivedComponent{T}"/> find component by the interface.
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Interface)]
    public sealed class DerivedFromAttribute : System.Attribute
    {
    }
}
