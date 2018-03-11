using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using CSharp;

namespace Urho3D
{
    public class StringVector : IList<string>, IDisposable
    {
        private class Enumerator : IEnumerator<string>
        {
            private StringVector instance_;
            private int index_;

            public Enumerator(StringVector instance)
            {
                instance_ = instance;
                Reset();
            }

            public bool MoveNext()
            {
                index_++;
                return instance_.Count > index_;
            }

            public void Reset()
            {
                index_ = -1;
            }

            public string Current => instance_[index_];

            object IEnumerator.Current => Current;

            public void Dispose()
            {
            }
        }

        private IntPtr instance_;

        internal StringVector(IntPtr instance)
        {
            instance_ = instance;
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
        public int Count => Urho3D_StringVector_Count(instance_);
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
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_StringVector_Add(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_StringVector_InsertAt(IntPtr instance, int index, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_StringVector_Set(IntPtr instance, int index, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.LPUTF8Str)]
        internal static extern string Urho3D_StringVector_Get(IntPtr instance, int index);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_StringVector_Remove(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_StringVector_RemoveAt(IntPtr instance, int index);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_StringVector_Clear(IntPtr instance);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_StringVector_Contains(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern int Urho3D_StringVector_Count(IntPtr instnace);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern int Urho3D_StringVector_IndexOf(IntPtr instance, [param: MarshalAs(UnmanagedType.LPUTF8Str)]string value);

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

        public void Dispose()
        {
        }
        #endregion
    }
}
