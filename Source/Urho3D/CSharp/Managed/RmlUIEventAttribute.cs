// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;

namespace Urho3DNet
{
    /// <summary>
    /// Attribute that forces generation of a RmlUI event for a given method.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    public sealed class RmlUIEventAttribute : System.Attribute
    {
    }
}
