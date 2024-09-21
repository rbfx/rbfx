using System;
using System.Linq;

namespace Urho3DNet
{
    public static class ObjectExtensionMethods
    {
        public static string GetFormattedTypeName(this Type type)
        {
            if (type.IsGenericType)
            {
                string genericArguments = type.GetGenericArguments()
                    .Select(GetFormattedTypeName)
                    .Aggregate((x1, x2) => $"{x1}, {x2}");
                return $"{type.Name.Substring(0, type.Name.IndexOf("`", StringComparison.Ordinal))}"
                       + $"<{genericArguments}>";
            }
            return type.Name;
        }
    }
}
