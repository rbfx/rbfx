using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace Urho3D.CSharp
{
    public static class ReflectionExtensions
    {
        public static IEnumerable<Tuple<Type, T>> GetTypesWithAttribute<T>(this Assembly assembly) {
            foreach(var type in assembly.GetTypes())
            {
                var attributes = type.GetCustomAttributes(typeof(T), true);
                if (attributes.Length > 0)
                    yield return new Tuple<Type, T>(type, (T)attributes[0]);
            }
        }
    }
}
