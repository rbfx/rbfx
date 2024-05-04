// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    public partial class ResourceRef
    {
        public static ResourceRef Parse(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                return null;

            int splitIndex = text.IndexOf(';');

            if (splitIndex < 0)
            {
                return new ResourceRef(text);
            }

            return new ResourceRef(text.Substring(0, splitIndex), text.Substring(splitIndex + 1));
        }

        public string ToString(Context context)
        {
            return $"{context.GetUrhoTypeName(this.Type)};{this.Name}";
        }
    }
}
