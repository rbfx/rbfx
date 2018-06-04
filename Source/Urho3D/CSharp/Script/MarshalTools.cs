//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
using System;
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
        private static StringUtf8 _instance = new StringUtf8();

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return _instance;
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            NativeInterface.Native.InteropFree(pNativeData);
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
            var pStr = NativeInterface.Native.InteropAlloc(s.Length + 1);

            Marshal.Copy(s, 0, pStr, s.Length);
            Marshal.WriteByte(pStr, s.Length, 0);

            return pStr;
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            var length = Marshal.ReadInt32(pNativeData, -4);
            return Encoding.UTF8.GetString((byte*) pNativeData, length - 1);
        }
    }

    public class PodArrayMarshaller<T> : ICustomMarshaler
    {
        private static PodArrayMarshaller<T> _instance = new PodArrayMarshaller<T>();

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return _instance;
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public unsafe IntPtr MarshalManagedToNative(object managedObj)
        {
            var array = (T[]) managedObj;
            var length = Marshal.SizeOf<T>() * array.Length;
            var result = NativeInterface.Native.InteropAlloc(length);

            var sourceHandle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                Buffer.MemoryCopy((void*) sourceHandle.AddrOfPinnedObject(), (void*) result, length, length);
                return result;
            }
            finally
            {
                sourceHandle.Free();
            }
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            var length = Marshal.ReadInt32(pNativeData, -4);
            var result = new T[length / Marshal.SizeOf<T>()];
            var resultHandle = GCHandle.Alloc(result, GCHandleType.Pinned);
            try
            {
                Buffer.MemoryCopy((void*) pNativeData, (void*) resultHandle.AddrOfPinnedObject(), length, length);
                return result;
            }
            finally
            {
                resultHandle.Free();
            }
        }
    }

    public class ObjArrayMarshaller<T> : ICustomMarshaler where T: NativeObject
    {
        private static ObjArrayMarshaller<T> _instance = new ObjArrayMarshaller<T>();

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return _instance;
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            var array = (T[]) managedObj;
            var length = IntPtr.Size * array.Length;
            var result = NativeInterface.Native.InteropAlloc(length);

            var offset = 0;
            foreach (var instance in array)
            {
                Marshal.WriteIntPtr(result, offset, instance.NativeInstance);
                offset += IntPtr.Size;
            }

            return result;
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            var length = Marshal.ReadInt32(pNativeData, -4);
            var result = new T[length / IntPtr.Size];
            var type = typeof(T);
            var getManaged = type.GetMethod("GetManagedInstance", BindingFlags.Public | BindingFlags.Static);
            for (var i = 0; i < result.Length; i++)
            {
                var instance = Marshal.ReadIntPtr(pNativeData, i * IntPtr.Size);
                result[i] = (T)getManaged.Invoke(null, new object[] {instance, false});
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
