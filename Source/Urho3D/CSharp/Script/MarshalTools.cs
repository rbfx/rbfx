using System;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    /// <summary>
    /// Marshals utf-8 strings. Native code controls lifetime of native string.
    /// </summary>
    public class StringUtf8 : ICustomMarshaler
    {
        [ThreadStatic]
        private static StringUtf8 _instance;
        private IntPtr _scratch = IntPtr.Zero;
        private int _scratchLength;

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return _instance ?? (_instance = new StringUtf8());
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
        }

        public int GetNativeDataSize()
        {
            return Marshal.SizeOf<IntPtr>();
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            if (!(managedObj is string))
                return IntPtr.Zero;

            var s = Encoding.UTF8.GetBytes((string) managedObj);

            if (_scratchLength < s.Length + 1)
            {
                _scratchLength = s.Length + 1;
                _scratch = Marshal.ReAllocHGlobal(_scratch, (IntPtr) _scratchLength);
            }

            Marshal.Copy(s, 0, _scratch, s.Length);
            Marshal.WriteByte(_scratch, s.Length, 0);
            return _scratch;
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            var length = 0;
            while (Marshal.ReadByte(pNativeData, length) != 0)
                ++length;
            return Encoding.UTF8.GetString((byte*) pNativeData, length);
        }
    }

    /// <summary>
    /// Marshals utf-8 strings. Managed code frees native string after obtaining it. Used in cases when native code
    /// returns by value.
    /// </summary>
    public class StringUtf8Copy : StringUtf8
    {
        [ThreadStatic]
        private static StringUtf8Copy _instance;

        public new static ICustomMarshaler GetInstance(string _)
        {
            return _instance ?? (_instance = new StringUtf8Copy());
        }

        public new void CleanUpNativeData(IntPtr pNativeData)
        {
        }

        public new object MarshalNativeToManaged(IntPtr pNativeData)
        {
            var result = base.MarshalNativeToManaged(pNativeData);
            NativeInterface.Native.FreeMemory(pNativeData);
            return result;
        }
    }

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
            var tFromPInvoke = type.GetMethod("GetManagedInstance", BindingFlags.NonPublic | BindingFlags.Static);
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
            var tToPInvoke = type.GetMethod("GetNativeInstance", BindingFlags.NonPublic | BindingFlags.Static);
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
            var tFromPInvoke = type.GetMethod("GetManagedInstance", BindingFlags.NonPublic | BindingFlags.Static);
            Debug.Assert(tFromPInvoke != null);
            var result = new TClass[array.Length];
            var i = 0;
            foreach (var element in array)
                result[i++] = (TClass) tFromPInvoke.Invoke(null, new object[] {element, false});
            return result;
        }
    }
}
