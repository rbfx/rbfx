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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using Urho3D;

namespace Urho3D.CSharp
{
    static class InstanceCache
    {
        /// <summary>
        /// How long (ms) strong reference is kept in CacheEntry after last access.
        /// </summary>
        private const int StrongRefExpirationTime = 10000;
        /// <summary>
        /// One iteration through entire cache is allowed during this interval (ms).
        /// </summary>
        private const int CacheIterationInterval = 30000;
        /// <summary>
        /// A map of types that inherit Urho3D.Object.
        /// </summary>
        private static readonly Dictionary<IntPtr, Type> _knownTypes = new Dictionary<IntPtr, Type>();

        static InstanceCache()
        {
            var objectType = typeof(NativeObject);
            var noParams = new object[] { };
            var noTypes = new Type[] { };
            foreach (var type in Assembly.GetCallingAssembly().GetTypes())
            {
                if (type == null)
                    continue;

                if (type.IsSubclassOf(objectType))
                {
                    var getTypeId = type.GetMethod("GetNativeTypeId", BindingFlags.NonPublic | BindingFlags.Static,
                        null, noTypes, null);
                    if (getTypeId != null)
                    {
                        var typeId = (IntPtr) getTypeId.Invoke(null, noParams);
                        _knownTypes[typeId] = type;
                    }
                }
            }
        }

        internal static Type GetNativeType(IntPtr typeId)
        {
            Type result;
            return _knownTypes.TryGetValue(typeId, out result) ? result : null;
        }

        internal class CacheEntry
        {
            // Strong reference is kept for some time after last object access and then set to null. This prevents
            // garbage-collection of often used objects whose reference is not kept around. When weak reference expires
            // entry is removed from cache.

            private NativeObject _target;
            private readonly WeakReference<NativeObject> _targetWeak;
            public int LastAccess;

            public NativeObject Target
            {
                get
                {
                    var result = _target;
                    if (result == null)
                        _targetWeak.TryGetTarget(out result);

                    if (result == null)
                        return null;

                    LastAccess = Environment.TickCount;
                    return result;
                }
            }

            public CacheEntry(NativeObject cachedTarget)
            {
                _target = cachedTarget;
                _targetWeak = new WeakReference<NativeObject>(cachedTarget);
                LastAccess = Environment.TickCount;
            }

            internal bool Expired
            {
                get
                {
                    if (_target != null && Environment.TickCount - LastAccess > StrongRefExpirationTime)
                        _target = null;
                    return !_targetWeak.TryGetTarget(out _);
                }
            }
        }

        private static Dictionary<IntPtr, CacheEntry> _cache = new Dictionary<IntPtr, CacheEntry>();

        public static T GetOrAdd<T>(IntPtr instance, Func<IntPtr, T> factory) where T: NativeObject
        {
            lock (_cache)
            {
                CacheEntry entry;
                if (!_cache.TryGetValue(instance, out entry))
                {
                    entry = new CacheEntry(factory(instance));
                    _needsReset = true;
                    _cache[instance] = entry;
                }

                entry.LastAccess = Environment.TickCount;
                ExpireCache();
                return (T)entry.Target;
            }
        }

        public static void Add<T>(T instance) where T: NativeObject
        {
            lock (_cache)
            {
                ExpireCache();
                _needsReset = true;
                _cache[instance.NativeInstance] = new CacheEntry(instance);
            }
        }

        public static void Remove(IntPtr instance)
        {
            lock (_cache)
            {
                _cache.Remove(instance);
            }
        }

        /// <summary>
        /// Dispose all cached references. Should be called on application exit before disposing Context.
        /// </summary>
        public static void Dispose()
        {
            lock (_cache)
            {
                var keyCollection = _cache.Keys;
                var keys = new IntPtr[keyCollection.Count];
                keyCollection.CopyTo(keys, 0);
                foreach (var key in keys)
                {
                    var entry = _cache[key];
                    entry.Target?.Dispose();
                }
                _cache.Clear();
            }
        }

        private static int _lastCacheEnumeratorResetTime = 0;
        private static bool _needsReset = true;
        private static IEnumerator<KeyValuePair<IntPtr, CacheEntry>> _expirationEnumerator;

        private static void ExpireCache()
        {
            if (_needsReset)
            {
                if (Environment.TickCount - _lastCacheEnumeratorResetTime < CacheIterationInterval)
                    return;
                _lastCacheEnumeratorResetTime = Environment.TickCount;
                _expirationEnumerator = _cache.GetEnumerator();
                _needsReset = false;
            }

            if (_expirationEnumerator.MoveNext())
            {
                var entry = _expirationEnumerator.Current;
                if (entry.Value.Expired)
                    Remove(entry.Key);
            }
            else
                _needsReset = true;
        }
    }
}
