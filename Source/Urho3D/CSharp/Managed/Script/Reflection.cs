// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Collections.Generic;
using System.Reflection;

namespace Urho3DNet.CSharp
{
    public static class ReflectionExtensions
    {
        public static IEnumerable<Tuple<Type, T>> GetTypesWithAttribute<T>(this Assembly assembly)
        {
            foreach(var type in assembly.GetTypes())
            {
                var attributes = type.GetCustomAttributes(typeof(T), true);
                foreach (var attribute in attributes)
                {
                    yield return new Tuple<Type, T>(type, (T) attribute);
                }
            }
        }

        public static IEnumerable<Tuple<MethodInfo, T>> GetMethodsWithAttribute<T>(this Assembly assembly)
        {
            foreach(var type in assembly.GetTypes())
            {
                foreach (var method in type.GetMethods())
                {
                    var attributes = method.GetCustomAttributes(typeof(T), true);
                    foreach (var attribute in attributes)
                    {
                        yield return new Tuple<MethodInfo, T>(method, (T) attribute);
                    }
                }
            }
        }
    }
}
