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
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Urho3D.CSharp;

namespace Urho3D
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct VectorBase
    {
        public uint Size;
        public uint Capacity;
        public IntPtr Buffer;
    }

    public unsafe class StringVector : NativeObject, IList<string>
    {
        private class Enumerator : IEnumerator<string>
        {
            private readonly StringVector _instance;
            private int _index;

            public Enumerator(StringVector instance)
            {
                _instance = instance;
                Reset();
            }

            public bool MoveNext()
            {
                _index++;
                return _instance.Count > _index;
            }

            public void Reset()
            {
                _index = -1;
            }

            public string Current => _instance[_index];

            object IEnumerator.Current => Current;

            public void Dispose()
            {
            }
        }

        internal StringVector(IntPtr instance, bool ownsInstnace)
        {
            SetupInstance(instance, ownsInstnace);
        }

        public IEnumerator<string> GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
        public void Add(string item) => Urho3D_StringVector_Add(NativeInstance, item);
        public void Clear() => Urho3D_StringVector_Clear(NativeInstance);
        public bool Contains(string item) => Urho3D_StringVector_Contains(NativeInstance, item);

        public void CopyTo(string[] array, int arrayIndex)
        {
            if (Count > array.Length - arrayIndex)
                throw new IndexOutOfRangeException();

            foreach (var value in this)
                array[arrayIndex++] = value;
        }

        public bool Remove(string item) => Urho3D_StringVector_Remove(NativeInstance, item);
        public int Count => (int)((VectorBase*) NativeInstance)->Size;
        public bool IsReadOnly => false;
        public int IndexOf(string item) => Urho3D_StringVector_IndexOf(NativeInstance, item);
        public void Insert(int index, string item) => Urho3D_StringVector_InsertAt(NativeInstance, index, item);
        public void RemoveAt(int index) => Urho3D_StringVector_RemoveAt(NativeInstance, index);

        public string this[int index]
        {
            get { return Urho3D_StringVector_Get(NativeInstance, index); }
            set { Urho3D_StringVector_Set(NativeInstance, index, value); }
        }

        #region Interop
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_Add(IntPtr instance,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_InsertAt(IntPtr instance, int index,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_Set(IntPtr instance, int index,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]
        private static extern string Urho3D_StringVector_Get(IntPtr instance, int index);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Urho3D_StringVector_Remove(IntPtr instance,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Urho3D_StringVector_RemoveAt(IntPtr instance, int index);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_Clear(IntPtr instance);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Urho3D_StringVector_Contains(IntPtr instance,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int Urho3D_StringVector_IndexOf(IntPtr instance,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_destructor(IntPtr instnace);

        internal static StringVector GetManagedInstance(IntPtr source, bool ownsInstnace)
        {
            return InstanceCache.GetOrAdd(source, ptr => new StringVector(ptr, ownsInstnace));
        }

        internal static IntPtr GetNativeInstance(StringVector source)
        {
            if (source == null)
            {
                return IntPtr.Zero;
            }
            return source.NativeInstance;
        }

        public override void Dispose()
        {
            if (Interlocked.Increment(ref DisposedCounter) == 1)
            {
                InstanceCache.Remove(NativeInstance);
                if (OwnsNativeInstance)
                    Urho3D_StringVector_destructor(NativeInstance);
            }
            NativeInstance = IntPtr.Zero;
        }

        #endregion
    }
}
