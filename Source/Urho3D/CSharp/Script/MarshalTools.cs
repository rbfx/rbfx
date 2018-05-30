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
        private IntPtr _scratch;
        private int _scratchLength;

        protected StringUtf8()
        {
            _scratchLength = 128;
            _scratch = Marshal.AllocHGlobal(_scratchLength);
        }

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

    [StructLayout(LayoutKind.Sequential)]
    internal struct SafeArray
    {
        public IntPtr Data;
        public int Length;

        [ThreadStatic] private static IntPtr _scratch;
        [ThreadStatic] private static int _scratchLength;

        static SafeArray()
        {
            _scratchLength = 128;
            _scratch = Marshal.AllocHGlobal(_scratchLength);
        }

        public static unsafe T[] GetManagedInstance<T>(SafeArray data, bool ownsNativeInstance)
        {
            if (data.Length == 0)
                return new T[0];

            var type = typeof(T);
            T[] result;
            if (type.IsValueType)
            {
                // Array of structs or builtin types.
                var size = Marshal.SizeOf<T>();
                result = new T[data.Length / size];
                var handle = GCHandle.Alloc(result, GCHandleType.Pinned);
                Buffer.MemoryCopy((void*) data.Data, (void*) handle.AddrOfPinnedObject(), data.Length, data.Length);
                handle.Free();
            }
            else
            {
                // Array of pointers to objects
                var tFromPInvoke = type.GetMethod("GetManagedInstance", BindingFlags.Public | BindingFlags.Static);
                var pointers = (void**) data.Data;
                Debug.Assert(pointers != null);
                Debug.Assert(tFromPInvoke != null);
                var count = data.Length / IntPtr.Size;
                result = new T[count];
                for (var i = 0; i < count; i++)
                    result[i] = (T) tFromPInvoke.Invoke(null, new object[] {new IntPtr(pointers[i]), false});
            }

            return result;
        }

        public static unsafe SafeArray GetNativeInstance<T>(T[] data)
        {
            if (data == null)
                return new SafeArray();

            var type = typeof(T);
            var length = data.Length * (type.IsValueType ? Marshal.SizeOf<T>() : IntPtr.Size);
            if (_scratchLength < length)
            {
                _scratchLength = length;
                _scratch = Marshal.ReAllocHGlobal(_scratch, (IntPtr) _scratchLength);
            }

            var result = new SafeArray
            {
                Length = length,
                Data = _scratch
            };

            if (type.IsValueType)
            {
                // Array of structs or builtin types.
                var handle = GCHandle.Alloc(result, GCHandleType.Pinned);
                Buffer.MemoryCopy((void*) handle.AddrOfPinnedObject(), (void*) result.Data, result.Length, result.Length);
                handle.Free();
            }
            else
            {
                // Array of pointers to objects
                var tToPInvoke = type.GetMethod("GetNativeInstance", BindingFlags.Public | BindingFlags.Static);
                var pointers = (void**) result.Length;
                Debug.Assert(tToPInvoke != null);
                Debug.Assert(pointers != null);
                for (var i = 0; i < data.Length; i++)
                    pointers[i] = ((IntPtr) tToPInvoke.Invoke(null, new object[] {data[i]})).ToPointer();
            }

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
    }
}
