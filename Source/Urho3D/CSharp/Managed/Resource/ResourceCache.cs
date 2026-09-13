// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    public partial class ResourceCache
    {
        public T GetResource<T>(string name, bool sendEventOnFailure = true) where T: Resource
        {
            if (string.IsNullOrEmpty(name))
                return null;
            return (T) GetResource(ObjectReflection<T>.TypeId, name, sendEventOnFailure);
        }

        public T GetTempResource<T>(string name, bool sendEventOnFailure = true) where T : Resource
        {
            if (string.IsNullOrEmpty(name))
                return null;
            return (T)GetTempResource(ObjectReflection<T>.TypeId, name, sendEventOnFailure);
        }
    }
}
