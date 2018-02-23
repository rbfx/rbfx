using System;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;

namespace CSharp
{
    internal static class MarshalTools
    {
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D__Free(IntPtr ptr);

        internal static string GetUtf8String(IntPtr ptr)
        {
            var len = 0;
            while (Marshal.ReadByte(ptr, len) != 0)
                len++;
            var buffer = new byte[len];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            return Encoding.UTF8.GetString(buffer);
        }

        internal static string GetUtf8StringFree(IntPtr ptr)
        {
            string result = GetUtf8String(ptr);
            Urho3D__Free(ptr);
            return result;
        }

        internal static bool HasOverride(this MethodInfo method)
        {
            return (method.Attributes & MethodAttributes.Virtual) != 0 &&
                   (method.Attributes & MethodAttributes.NewSlot) == 0;
        }

        internal static bool HasOverride(this Type type, string methodName, params Type[] paramTypes)
        {
            var method = type.GetMethod(methodName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null,
                CallingConventions.HasThis, paramTypes, new ParameterModifier[0]);
            return method != null && method.HasOverride();
        }
    }
}
