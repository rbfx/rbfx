// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Text;

namespace Urho3DNet
{
    public partial class ResourceRefList
    {
        public static ResourceRefList Parse(string value)
        {
            return string.IsNullOrWhiteSpace(value) ? default : Urho3D.ToResourceRefList(value);
        }
    }
}
