using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using Xunit;

namespace Urho3DNet.Tests
{
    public class CodeAnalysis
    {
        private static bool HasUnknownTypeArg(MethodInfo method)
        {
            return method.GetParameters()
                .Any(parameterInfo => parameterInfo.ParameterType.Name.StartsWith("SWIGTYPE_"));
        }

        [Fact(Skip = "Needs some further cleanup in SWIG config")]
        public void NoUnknownTypesExposed()
        {
            var assembly = typeof(Vector2).Assembly;
            StringBuilder report = new StringBuilder();

            HashSet<string> visitedMembers = new HashSet<string>();

            foreach (var type in assembly.GetTypes())
            {
                HashSet<MethodInfo> methodToIgnore = new HashSet<MethodInfo>();
                foreach (var propertyInfo in type.GetProperties())
                {
                    foreach (var methodInfo in propertyInfo.GetAccessors())
                    {
                        methodToIgnore.Add(methodInfo);
                    }
                }

                foreach (var methodInfo in type.GetMethods())
                {
                    if (methodInfo.DeclaringType != type)
                        continue;
                    if (methodToIgnore.Contains(methodInfo))
                        continue;

                    if (HasUnknownTypeArg(methodInfo))
                    {
                        var hasSafeOverrides = type.GetMethods().Where(_ => methodInfo.DeclaringType == type).Any(_ => _.Name == methodInfo.Name && !HasUnknownTypeArg(_));
                        var prefix = hasSafeOverrides ? "// " : "";

                        var memberName = $"Urho3D::{type.Name}::{methodInfo.Name}";
                        if (visitedMembers.Add(memberName))
                        {
                            report.AppendLine($"{prefix}%ignore {memberName};");
                        }

                        break;
                    }
                }
            }

            if (report.Length > 0)
                Assert.Fail(report.ToString());
        }
    }
}
