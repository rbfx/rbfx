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
        public uint size_;
        public uint capacity_;
        public IntPtr buffer_;
    }

    public unsafe class BasePODVector
    {
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Urho3D_VectorBase_AllocateBuffer(int size);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_VectorBase_FreeBuffer(IntPtr buffer);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern VectorBase* Urho3D_VectorBase_VectorBase();
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_VectorBase_destructor(VectorBase* instance);
    }

    public unsafe class StringVector : IList<string>, IDisposable
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
        public int Count => (int)((VectorBase*) instance_)->size_;
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

    public unsafe class PODVector<T> : BasePODVector, IList<T>, IDisposable
    {
        private VectorBase* instance_;
        private static int typeSize_
        {
            get
            {
                var type = typeof(T);
                if (type.IsValueType)
                    return Marshal.SizeOf<T>();
                return IntPtr.Size;
            }
        }

        internal PODVector(VectorBase* instance)
        {
            instance_ = instance;
        }

        public PODVector()
        {
            instance_ = Urho3D_VectorBase_VectorBase();
        }

        public PODVector(T[] items)
        {
            instance_ = Urho3D_VectorBase_VectorBase();
            instance_->capacity_ = instance_->size_ = (uint) items.Length;
            if (Count > 0)
            {
                instance_->buffer_ = Urho3D_VectorBase_AllocateBuffer(Count);

                var type = typeof(T);
                if (type.IsPrimitive)
                {
                    var handle = GCHandle.Alloc(items, GCHandleType.Pinned);
                    var source = handle.AddrOfPinnedObject().ToPointer();
                    var len = instance_->size_ * typeSize_;
                    Buffer.MemoryCopy(source, (void*)instance_->buffer_, len, len);
                    handle.Free();
                }
                else if (type.IsValueType)
                {
                    for (var i = 0; i < Count; i++)
                    {
                        var p = new IntPtr((byte*)instance_->buffer_ + i * typeSize_);
                        Marshal.StructureToPtr(items[i], p, false);
                    }
                }
                else
                    throw new ArgumentException("PODVector can be constructed only from value types.");
            }
        }

        private class Enumerator : IEnumerator<T>
        {
            private PODVector<T> instance_;
            private int index_;

            public Enumerator(PODVector<T> instance)
            {
                instance_ = instance;
                Reset();
            }

            public bool MoveNext()
            {
                index_++;
                return index_ < instance_.Count;
            }

            public void Reset()
            {
                index_ = -1;
            }

            public T Current => instance_[index_];

            object IEnumerator.Current => Current;

            public void Dispose()
            {
            }
        }

        public IEnumerator<T> GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

        public void Add(T item)
        {
            throw new NotImplementedException();
        }

        public void Clear()
        {
            throw new NotImplementedException();
        }

        public bool Contains(T item)
        {
            foreach (var value in this)
            {
                if (value.Equals(item))
                    return true;
            }
            return false;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            if (array.Length < Count - arrayIndex)
                throw new IndexOutOfRangeException();

            // TODO: Can be optimized.
            foreach (var value in this)
                array[arrayIndex++] = value;
        }

        public bool Remove(T item)
        {
            throw new NotImplementedException();
        }

        public int Count => (int) instance_->size_;
        public bool IsReadOnly => true;
        public int IndexOf(T item)
        {
            int index = 0;
            foreach (var value in this)
            {
                if (value.Equals(item))
                    return index;
            }
            return -1;
        }

        public void Insert(int index, T item)
        {
            throw new NotImplementedException();
        }

        public void RemoveAt(int index)
        {
            throw new NotImplementedException();
        }

        public T this[int index]
        {
            get
            {
                if (index >= Count)
                    throw new IndexOutOfRangeException();

                var type = typeof(T);
                var itemAddress = (byte*) instance_->buffer_ + index * typeSize_;
                if (type.IsPrimitive)
                {
                    T result = default(T);
                    var handle = GCHandle.Alloc(result);
                    Buffer.MemoryCopy(itemAddress, (void*)handle.AddrOfPinnedObject(), typeSize_, typeSize_);
                    handle.Free();
                    return result;
                }
                else if (type.IsValueType)
                {
                    return Marshal.PtrToStructure<T>((IntPtr)itemAddress);
                }
                else
                {
                    // Array of pointers
                    var fromPInvoke = typeof(T).GetMethod("__FromPInvoke", BindingFlags.NonPublic | BindingFlags.Static);
                    Debug.Assert(fromPInvoke != null);
                    return (T)fromPInvoke.Invoke(null, new object[] {new IntPtr(*(void**)itemAddress) });
                }
            }
            set
            {
                if (index >= Count)
                    throw new IndexOutOfRangeException();
                var type = typeof(T);
                var itemAddress = (byte*) instance_->buffer_ + index * typeSize_;

                if (type.IsPrimitive)
                {
                    var handle = GCHandle.Alloc(value);
                    Buffer.MemoryCopy((void*)handle.AddrOfPinnedObject(), itemAddress, typeSize_, typeSize_);
                    handle.Free();
                }
                else if (type.IsValueType)
                {
                    Marshal.StructureToPtr<T>(value, (IntPtr)itemAddress, false);
                }
                else
                {
                    // Array of pointers
                    var toPInvoke = typeof(T).GetMethod("__ToPInvoke", BindingFlags.NonPublic | BindingFlags.Static);
                    Debug.Assert(toPInvoke != null);
                    var ptr = (IntPtr)toPInvoke.Invoke(null, new object[] {value});
                    *(IntPtr*)itemAddress = ptr;
                }
            }
        }

        internal static VectorBase* __ToPInvoke(PODVector<T> instance)
        {
            if (instance == null)
                return null;
            return instance.instance_;
        }

        internal static PODVector<T> __FromPInvoke(VectorBase* instance)
        {
            if (instance == null)
                return null;
            return InstanceCache.GetOrAdd((IntPtr) instance, ptr => new PODVector<T>((VectorBase*) ptr));
        }

        protected volatile int disposed_;
        public void Dispose()
        {
            if (Interlocked.Increment(ref disposed_) == 1)
            {
                Urho3D_VectorBase_destructor(instance_);
                InstanceCache.Remove((IntPtr)instance_, this);
            }
            instance_ = null;
        }

        ~PODVector()
        {
            Dispose();
        }
    }
}
