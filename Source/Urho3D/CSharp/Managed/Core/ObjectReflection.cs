// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Reflection;

namespace Urho3DNet
{
    /// <summary>
    /// Object reflection type that caches type information for a given type.
    /// </summary>
    public static class ObjectReflection<T>
    {
        public static string ClassName { get; }

        public static StringHash TypeId { get; }

        static ObjectReflection()
        {
            var type = typeof(T);
            var getTypeNameStatic = type.GetMethod(nameof(Urho3DNet.Object.GetTypeNameStatic), BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
            if (getTypeNameStatic != null)
            {
                ClassName = (string)getTypeNameStatic.Invoke(null, Array.Empty<object>());
            }
            else
            {
                ClassName = type.Name;
            }

            var getTypeStatic = type.GetMethod(nameof(Urho3DNet.Object.GetTypeStatic), BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
            if (getTypeStatic != null)
            {
                TypeId = (StringHash)getTypeStatic.Invoke(null, Array.Empty<object>());
            }
            else
            {
                TypeId = ClassName;
            }
        }
    }
}
