// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    public partial class Component
    {
        public T GetComponent<T>() where T : Component
        {
            return (T)GetComponent(ObjectReflection<T>.TypeId);
        }
    }
}
