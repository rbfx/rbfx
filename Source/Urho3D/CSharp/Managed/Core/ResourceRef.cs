// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;

namespace Urho3DNet
{
    public partial class ResourceRef
    {
        private static readonly Lazy<ResourceRef> _empty = new Lazy<ResourceRef>(() => new ResourceRef(new StringHash(0u), string.Empty));

        public static ResourceRef Empty => _empty.Value;

        public static ResourceRef Parse(string value)
        {
            return string.IsNullOrWhiteSpace(value) ? default : Urho3D.ToResourceRef(value);
        }

        public static bool IsNullOrEmpty(ResourceRef resourceRef)
        {
            return resourceRef == null || string.IsNullOrEmpty(resourceRef.Name);
        }
    }
}
