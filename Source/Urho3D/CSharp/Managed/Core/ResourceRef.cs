// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    public partial class ResourceRef
    {
        public static ResourceRef Parse(string value)
        {
            return string.IsNullOrWhiteSpace(value) ? default : Urho3D.ToResourceRef(value);
        }
    }
}
