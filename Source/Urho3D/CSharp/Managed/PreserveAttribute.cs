// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    /// <summary>
    /// Preserve attributes required to keep Urho3DNet classes in assembly on some platforms like iOS.
    /// </summary>
    public sealed class PreserveAttribute : System.Attribute
    {
        public bool AllMembers;
        public bool Conditional;
    }
}
