using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Urho3D.CSharp;

namespace Urho3D
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct HashIteratorBase
    {
        public IntPtr ptr_;
    }

    public class VariantMap : NativeObject, IDictionary<StringHash, Variant>
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern uint Urho3D_HashMap_StringHash_Variant_GetKey(HashIteratorBase it);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr Urho3D_HashMap_StringHash_Variant_GetValue(HashIteratorBase it);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Urho3D_HashMap_StringHash_Variant_Add(IntPtr map, uint key, IntPtr value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool Urho3D_HashMap_StringHash_Variant_Remove(IntPtr map, uint key);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool Urho3D_HashMap_StringHash_Variant_First(IntPtr map, out HashIteratorBase it);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool Urho3D_HashMap_StringHash_Variant_Next(IntPtr map, ref HashIteratorBase it);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool Urho3D_HashMap_StringHash_Variant_Contains(IntPtr map, uint key);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr Urho3D_HashMap_StringHash_Variant_TryGet(IntPtr map, uint key);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Urho3D_HashMap_StringHash_Variant_destructor(IntPtr map);

        class Enumerator : IEnumerator<KeyValuePair<StringHash, Variant>>
        {
            private readonly VariantMap _hashMap;
            private HashIteratorBase _iterator;
            private bool _isFirst = true;

            public Enumerator(VariantMap hashMap)
            {
                _hashMap = hashMap;
                Reset();
            }

            public bool MoveNext()
            {
                if (_isFirst)
                {
                    _isFirst = false;
                    return Urho3D_HashMap_StringHash_Variant_First(_hashMap.NativeInstance, out _iterator);
                }
                return Urho3D_HashMap_StringHash_Variant_Next(_hashMap.NativeInstance, ref _iterator);
            }

            public void Reset()
            {
                _isFirst = true;
            }

            public KeyValuePair<StringHash, Variant> Current
            {
                get
                {
                    var rawKey = Urho3D_HashMap_StringHash_Variant_GetKey(_iterator);
                    var rawVal = Urho3D_HashMap_StringHash_Variant_GetValue(_iterator);
                    var key = StringHash.__FromPInvoke(rawKey, false);
                    var value = Variant.__FromPInvoke(rawVal, false);
                    return new KeyValuePair<StringHash, Variant>(key, value);
                }
            }

            object IEnumerator.Current => Current;

            public void Dispose()
            {
            }
        }

        internal VariantMap(IntPtr instance, bool ownsInstnace)
        {
            SetupInstance(instance, ownsInstnace);
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
            Urho3D_HashMap_StringHash_Variant_Add(NativeInstance, pInvokeKey, pInvokeValue);
        }

        public bool ContainsKey(StringHash key)
        {
            return Urho3D_HashMap_StringHash_Variant_Contains(NativeInstance, key.Hash);
        }

        public bool Remove(StringHash key)
        {
            return Urho3D_HashMap_StringHash_Variant_Remove(NativeInstance, key.Hash);
        }

        public bool TryGetValue(StringHash key, out Variant value)
        {
            var instance = Urho3D_HashMap_StringHash_Variant_TryGet(NativeInstance, key.Hash);
            if (instance == IntPtr.Zero)
            {
                value = null;
                return false;
            }
            value = Variant.__FromPInvoke(instance, true);
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

        internal static VariantMap __FromPInvoke(IntPtr source, bool ownsNativeInstnace)
        {
            return InstanceCache.GetOrAdd<VariantMap>(source, ptr => new VariantMap(ptr, ownsNativeInstnace));
        }

        internal static IntPtr __ToPInvoke(VariantMap source)
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
                    Urho3D_HashMap_StringHash_Variant_destructor(NativeInstance);
            }
            NativeInstance = IntPtr.Zero;
        }
    }
}
