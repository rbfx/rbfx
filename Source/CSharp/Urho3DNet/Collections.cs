using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Urho3D
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct HashIteratorBase
    {
        public IntPtr ptr_;
    }

    public class VariantMap : IDictionary<StringHash, Variant>
    {
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint Urho3D_HashMap_StringHash_Variant_GetKey(HashIteratorBase it);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Urho3D_HashMap_StringHash_Variant_GetValue(HashIteratorBase it);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_HashMap_StringHash_Variant_Add(IntPtr map, uint key, IntPtr value);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_HashMap_StringHash_Variant_Remove(IntPtr map, uint key);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_HashMap_StringHash_Variant_First(IntPtr map, out HashIteratorBase it);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_HashMap_StringHash_Variant_Next(IntPtr map, ref HashIteratorBase it);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern bool Urho3D_HashMap_StringHash_Variant_Contains(IntPtr map, uint key);
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Urho3D_HashMap_StringHash_Variant_TryGet(IntPtr map, uint key);

        class Enumerator : IEnumerator<KeyValuePair<StringHash, Variant>>
        {
            private readonly VariantMap hashMap_;
            private HashIteratorBase iterator_;
            private bool isFirst_ = true;

            public Enumerator(VariantMap hashMap)
            {
                hashMap_ = hashMap;
                Reset();
            }

            public bool MoveNext()
            {
                if (isFirst_)
                {
                    isFirst_ = false;
                    return Urho3D_HashMap_StringHash_Variant_First(hashMap_.instance_, out iterator_);
                }
                return Urho3D_HashMap_StringHash_Variant_Next(hashMap_.instance_, ref iterator_);
            }

            public void Reset()
            {
                isFirst_ = true;
            }

            public KeyValuePair<StringHash, Variant> Current
            {
                get
                {
                    var rawKey = Urho3D_HashMap_StringHash_Variant_GetKey(iterator_);
                    var rawVal = Urho3D_HashMap_StringHash_Variant_GetValue(iterator_);
                    var key = StringHash.__FromPInvoke(rawKey);
                    var value = Variant.__FromPInvoke(rawVal);
                    return new KeyValuePair<StringHash, Variant>(key, value);
                }
            }

            object IEnumerator.Current => Current;

            public void Dispose()
            {
            }
        }

        internal IntPtr instance_;

        internal VariantMap(IntPtr instance)
        {
            instance_ = instance;
        }

        public IEnumerator<KeyValuePair<StringHash, Variant>> GetEnumerator()
        {
            return new Enumerator(this);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public void Add(KeyValuePair<StringHash, Variant> item)
        {
            Add(item.Key, item.Value);
        }

        public void Clear()
        {
        }

        public bool Contains(KeyValuePair<StringHash, Variant> item)
        {
            return this.Any(kv => kv.Key.Equals(item.Key) && kv.Value.Equals(item.Value));
        }

        public void CopyTo(KeyValuePair<StringHash, Variant>[] array, int arrayIndex)
        {
            foreach (var kv in this)
                array[arrayIndex++] = kv;
        }

        public bool Remove(KeyValuePair<StringHash, Variant> item)
        {
            return Contains(item) && Remove(item.Key);
        }

        public int Count
        {
            get
            {
                int count = 0;
                using (var enumerator = GetEnumerator())
                    for (; enumerator.MoveNext(); count++) { }
                return count;
            }
        }

        public bool IsReadOnly => false;
        public void Add(StringHash key, Variant value)
        {
            var pInvokeKey = StringHash.__ToPInvoke(key);
            var pInvokeValue = Variant.__ToPInvoke(value);
            Urho3D_HashMap_StringHash_Variant_Add(instance_, pInvokeKey, pInvokeValue);
        }

        public bool ContainsKey(StringHash key)
        {
            return Urho3D_HashMap_StringHash_Variant_Contains(instance_, key.Hash);
        }

        public bool Remove(StringHash key)
        {
            return Urho3D_HashMap_StringHash_Variant_Remove(instance_, key.Hash);
        }

        public bool TryGetValue(StringHash key, out Variant value)
        {
            var instance = Urho3D_HashMap_StringHash_Variant_TryGet(instance_, key.Hash);
            if (instance == IntPtr.Zero)
            {
                value = null;
                return false;
            }
            value = Variant.__FromPInvoke(instance);
            return true;
        }

        public Variant this[StringHash key]
        {
            get
            {
                Variant value;
                if (TryGetValue(key, out value))
                    return value;
                throw new KeyNotFoundException();
            }
            set { Add(key, value); }
        }

        public ICollection<StringHash> Keys => this.Select(kv => kv.Key).Distinct().ToList();
        public ICollection<Variant> Values => this.Select(kv => kv.Value).Distinct().ToList();
    }
}
