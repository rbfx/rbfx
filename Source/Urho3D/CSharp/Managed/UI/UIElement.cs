// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Collections.Generic;
using System.Text;

namespace Urho3DNet
{
    public partial class UIElement
    {
        public T CreateChild<T>() where T : UIElement
        {
            return (T)CreateChild(ObjectReflection<T>.TypeId);
        }

        public T CreateChild<T>(string name) where T : UIElement
        {
            return (T)CreateChild(ObjectReflection<T>.TypeId, name);
        }

        public T CreateChild<T>(string name, uint index) where T : UIElement
        {
            return (T)CreateChild(ObjectReflection<T>.TypeId, name, index);
        }

        public T GetChild<T>(string name, bool recursive) where T : UIElement
        {
            return (T) GetChild(ObjectReflection<T>.TypeId, name, recursive);
        }
    }
}
