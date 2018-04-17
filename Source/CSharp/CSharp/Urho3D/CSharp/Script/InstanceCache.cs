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
                    NativeObject result;
                    return !_targetWeak.TryGetTarget(out result);
                }
            }
        }

        private static ConcurrentDictionary<IntPtr, CacheEntry> _cache = new ConcurrentDictionary<IntPtr, CacheEntry>();
        private static IEnumerator<KeyValuePair<IntPtr, CacheEntry>> _expirationEnumerator;
        private static int _lastCacheEnumeratorResetTime = 0;
        private static bool _needsReset = true;

        public static T GetOrAdd<T>(IntPtr instance, Func<IntPtr, T> factory) where T: NativeObject
        {
            var entry = _cache.GetOrAdd(instance, ptr =>
            {
                var object_ = factory(ptr);
                return new CacheEntry(object_);
            });
            entry.LastAccess = Environment.TickCount;
            ExpireCache();
            return (T)entry.Target;
        }

        public static void Add<T>(T instance) where T: NativeObject
        {
            ExpireCache();
            _cache[instance.NativeInstance] = new CacheEntry(instance);
        }

        public static void Remove(IntPtr instance)
        {
            ExpireCache();
            CacheEntry entry;
            if (!_cache.TryRemove(instance, out entry))
                return;

            if (entry.Target is Context)
                NativeInterface.Dispose();
        }

        /// <summary>
        /// Dispose all cached references. Should be called on application exit before disposing Context.
        /// </summary>
        public static void Dispose()
        {
            foreach (var item in _cache)
                item.Value.Target?.Dispose();
            _cache.Clear();
        }

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
