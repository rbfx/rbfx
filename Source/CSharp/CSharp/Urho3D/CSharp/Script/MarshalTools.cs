using System;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    internal static class MarshalTools
    {
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

        internal static IntPtr[] ToIntPtrArray<T>(T[] objects) where T : NativeObject
        {
            var array = new IntPtr[objects.Length];
            var i = 0;
            foreach (var instance in objects)
                array[i++] = instance.NativeInstance;
            return array;
        }

        internal static T[] ToObjectArray<T>(IntPtr[] objects) where T : NativeObject
        {
            var type = typeof(T);
            var tFromPInvoke = type.GetMethod("__FromPInvoke", BindingFlags.NonPublic | BindingFlags.Static);
            Debug.Assert(tFromPInvoke != null);

            var array = new T[objects.Length];
            var i = 0;
            foreach (var instance in objects)
                array[i++] = (T)tFromPInvoke.Invoke(null, new object[]{instance, false});
            return array;
        }

        internal static TBuiltin[] ToPInvokeArray<TBuiltin, TClass>(TClass[] array)
        {
            var type = typeof(TClass);
            var tToPInvoke = type.GetMethod("__ToPInvoke", BindingFlags.NonPublic | BindingFlags.Static);
            Debug.Assert(tToPInvoke != null);
            var result = new TBuiltin[array.Length];
            var i = 0;
            foreach (var element in array)
                result[i++] = (TBuiltin) tToPInvoke.Invoke(null, new object[] {element});
            return result;
        }

        internal static TClass[] ToCSharpArray<TBuiltin, TClass>(TBuiltin[] array)
        {
            var type = typeof(TClass);
            var tFromPInvoke = type.GetMethod("__FromPInvoke", BindingFlags.NonPublic | BindingFlags.Static);
            Debug.Assert(tFromPInvoke != null);
            var result = new TClass[array.Length];
            var i = 0;
            foreach (var element in array)
                result[i++] = (TClass) tFromPInvoke.Invoke(null, new object[] {element, false});
            return result;
        }
    }
}
