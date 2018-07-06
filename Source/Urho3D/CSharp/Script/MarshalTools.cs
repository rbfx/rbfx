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
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    /// <summary>
    /// Marshals utf-8 strings. Native code controls lifetime of native string.
    /// </summary>
    public struct StringUtf8 : ICustomMarshaler
    {
        private IntPtr _initialNativeData;

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return new StringUtf8();
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            if (_initialNativeData != IntPtr.Zero && _initialNativeData != pNativeData)
                NativeInterface.Native.InteropFree(_initialNativeData);

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            if (!(managedObj is string))
                return IntPtr.Zero;

            var str = Encoding.UTF8.GetBytes((string) managedObj);
            var memory = NativeInterface.Native.InteropAlloc(str.Length + 1);

            Marshal.Copy(str, 0, memory, str.Length);
            Marshal.WriteByte(memory, str.Length, 0);

            _initialNativeData = memory;
            return memory;
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return null;

            var length = NativeInterface.GetLengthOfAllocatedMemory(pNativeData);
            return Encoding.UTF8.GetString((byte*) pNativeData, length - 1);
        }
    }

    /// <summary>
    /// Marshals array of utf8 strings to char**. Each string is null-terminated and array is terminated with a null ptr.
    /// </summary>
    public struct StringUtf8ArrayMarshaller : ICustomMarshaler
    {
        private IntPtr _initialNativeData;

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return new StringUtf8ArrayMarshaller();
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            if (_initialNativeData != IntPtr.Zero && _initialNativeData != pNativeData)
                NativeInterface.Native.InteropFree(_initialNativeData);

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            var strings = (string[]) managedObj;
            var utf8Strings = new byte[strings.Length][];
            var stringPtrArraySize = (strings.Length + 1) * IntPtr.Size;
            var bufferLength = stringPtrArraySize;    // Array of pointers to strings + null terminator
            var index = 0;
            foreach (var str in strings)
            {
                var utf8 = Encoding.UTF8.GetBytes(str);
                bufferLength += utf8.Length + 1;
                utf8Strings[index++] = utf8;
            }

            var buffer = NativeInterface.Native.InteropAlloc(bufferLength);

            var stringData = buffer + stringPtrArraySize;
            for (var i = 0; i < strings.Length; i++)
            {
                // Pointer to string
                Marshal.WriteIntPtr(buffer, i * IntPtr.Size, stringData);
                // String data after array of pointers
                var str = utf8Strings[i];
                Marshal.Copy(str, 0, stringData, str.Length);
                // Null terminator
                Marshal.WriteByte(stringData, str.Length, 0);
                stringData += str.Length + 1;
            }
            // Terminate array of strings
            Marshal.WriteIntPtr(buffer, strings.Length * IntPtr.Size, IntPtr.Zero);

            _initialNativeData = buffer;
            return buffer;
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return null;

            // Count number of array items
            var count = 0;
            while (Marshal.ReadIntPtr(pNativeData, count * IntPtr.Size) != IntPtr.Zero)
                count++;

            var strings = new string[count];

            for (var i = 0; i < count; i++)
            {
                var stringData = Marshal.ReadIntPtr(pNativeData, i * IntPtr.Size);
                var length = NativeInterface.StrLen(stringData);
                strings[i] = Encoding.UTF8.GetString((byte*) stringData, length);
            }

            return strings;
        }
    }

    public struct PodArrayMarshaller<T> : ICustomMarshaler where T: struct
    {
        private IntPtr _initialNativeData;

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return new PodArrayMarshaller<T>();
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            if (_initialNativeData != IntPtr.Zero && _initialNativeData != pNativeData)
                NativeInterface.Native.InteropFree(_initialNativeData);

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public unsafe IntPtr MarshalManagedToNative(object managedObj)
        {
            if (managedObj == null)
                return IntPtr.Zero;

            var array = (T[]) managedObj;
            if (array.Length == 0)
                return IntPtr.Zero;

            var itemSize = Marshal.SizeOf<T>();
            var length = itemSize * array.Length;
            var memory = NativeInterface.Native.InteropAlloc(length);
            var sourceHandle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                Buffer.MemoryCopy((void*) sourceHandle.AddrOfPinnedObject(), (void*) memory, length, length);
                _initialNativeData = memory;
                return memory;
            }
            finally
            {
                sourceHandle.Free();
            }
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return null;

            var memoryLength = NativeInterface.GetLengthOfAllocatedMemory(pNativeData);
            var result = new T[memoryLength / Marshal.SizeOf<T>()];
            var resultHandle = GCHandle.Alloc(result, GCHandleType.Pinned);
            try
            {
                Buffer.MemoryCopy((void*) pNativeData, (void*) resultHandle.AddrOfPinnedObject(), memoryLength,
                    memoryLength);
                return result;
            }
            finally
            {
                resultHandle.Free();
            }
        }
    }

    public struct ObjPtrArrayMarshaller<T> : ICustomMarshaler where T: NativeObject
    {
        private IntPtr _initialNativeData;

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return new ObjPtrArrayMarshaller<T>();
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            if (_initialNativeData != IntPtr.Zero && _initialNativeData != pNativeData)
                NativeInterface.Native.InteropFree(_initialNativeData);

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            if (managedObj == null)
                return IntPtr.Zero;

            var array = (T[]) managedObj;
            if (array.Length == 0)
                return IntPtr.Zero;

            var length = IntPtr.Size * array.Length;
            var memory = NativeInterface.Native.InteropAlloc(length);

            for (var i = 0; i < array.Length; i++)
                Marshal.WriteIntPtr(memory, i * IntPtr.Size, array[i].NativeInstance);

            _initialNativeData = memory;
            return memory;
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return new T[0];

            var result = new T[NativeInterface.GetLengthOfAllocatedMemory(pNativeData) / IntPtr.Size];
            var type = typeof(T);
            var getManaged = type.GetMethod("GetManagedInstance", BindingFlags.NonPublic | BindingFlags.Static);
            for (var i = 0; i < result.Length; i++)
            {
                var instance = Marshal.ReadIntPtr(pNativeData, i * IntPtr.Size);
                result[i] = (T)getManaged.Invoke(null, new object[] {instance, NativeObjectFlags.None});
            }

            return result;
        }
    }

    public struct ObjArrayMarshaller<T> : ICustomMarshaler where T: NativeObject
    {
        private IntPtr _initialNativeData;

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return new ObjArrayMarshaller<T>();
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return;

            if (_initialNativeData != IntPtr.Zero && _initialNativeData != pNativeData)
                NativeInterface.Native.InteropFree(_initialNativeData);

            NativeInterface.Native.InteropFree(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            // _initialNativeData = ...;
            throw new NotImplementedException();
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return new T[0];

            var type = typeof(T);
            var getManaged = type.GetMethod("GetManagedInstance", BindingFlags.NonPublic | BindingFlags.Static);
            var getSize = type.GetMethod("GetNativeTypeSize", BindingFlags.NonPublic | BindingFlags.Static);
            var sizeOfItem = (int) getSize.Invoke(null, new object[0]);
            var result = new T[NativeInterface.GetLengthOfAllocatedMemory(pNativeData) / sizeOfItem];

            for (var i = 0; i < result.Length; i++)
            {
                var instance = pNativeData + i * sizeOfItem;
                result[i] = (T)getManaged.Invoke(null, new object[] {instance, NativeObjectFlags.NonOwningReference});
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
