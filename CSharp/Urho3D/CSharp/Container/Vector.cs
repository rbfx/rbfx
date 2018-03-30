using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using CSharp;

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

        internal StringVector(IntPtr instance)
        {
            SetupInstance(instance);
        }

        public IEnumerator<string> GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
        public void Add(string item) => Urho3D_StringVector_Add(instance_, item);
        public void Clear() => Urho3D_StringVector_Clear(instance_);
        public bool Contains(string item) => Urho3D_StringVector_Contains(instance_, item);

        public void CopyTo(string[] array, int arrayIndex)
        {
            if (Count > array.Length - arrayIndex)
                throw new IndexOutOfRangeException();

            foreach (var value in this)
                array[arrayIndex++] = value;
        }

        public bool Remove(string item) => Urho3D_StringVector_Remove(instance_, item);
        public int Count => (int)((VectorBase*) instance_)->Size;
        public bool IsReadOnly => false;
        public int IndexOf(string item) => Urho3D_StringVector_IndexOf(instance_, item);
        public void Insert(int index, string item) => Urho3D_StringVector_InsertAt(instance_, index, item);
        public void RemoveAt(int index) => Urho3D_StringVector_RemoveAt(instance_, index);

        public string this[int index]
        {
            get { return Urho3D_StringVector_Get(instance_, index); }
            set { Urho3D_StringVector_Set(instance_, index, value); }
        }

        #region Interop
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_Add(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_InsertAt(IntPtr instance, int index, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_Set(IntPtr instance, int index, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.LPUTF8Str)]
        private static extern string Urho3D_StringVector_Get(IntPtr instance, int index);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Urho3D_StringVector_Remove(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Urho3D_StringVector_RemoveAt(IntPtr instance, int index);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_Clear(IntPtr instance);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool Urho3D_StringVector_Contains(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int Urho3D_StringVector_IndexOf(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_StringVector_destructor(IntPtr instnace);

        internal static StringVector __FromPInvoke(IntPtr source)
        {
            return InstanceCache.GetOrAdd<StringVector>(source, ptr => new StringVector(ptr));
        }

        internal static IntPtr __ToPInvoke(StringVector source)
        {
            if (source == null)
            {
                return IntPtr.Zero;
            }
            return source.instance_;
        }

        public override void Dispose()
        {
            if (Interlocked.Increment(ref disposed_) == 1)
            {
                InstanceCache.Remove(instance_);
                Urho3D_StringVector_destructor(instance_);
            }
            instance_ = IntPtr.Zero;
        }

        #endregion
    }
}
