using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using Urho3D;

namespace CSharp
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

        internal class CacheEntry
        {
            // Strong reference is kept for some time after last object access and then set to null. This prevents
            // garbage-collection of often used objects whose reference is not kept around. When weak reference expires
            // entry is removed from cache.

            private NativeObject _target;
            private WeakReference<NativeObject> _targetWeak;
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
            ExpireCache();
            var entry = _cache.GetOrAdd(instance, ptr =>
            {
                var object_ = (NativeObject) factory(ptr);
                // In case this is RefCounted object add a reference for duration of object's lifetime.
                (object_ as RefCounted)?.AddRef();
                return new CacheEntry(object_);
            });
            entry.LastAccess = Environment.TickCount;
            return (T)entry.Target;
        }

        public static void Add<T>(T instance) where T: NativeObject
        {
            ExpireCache();
            if (instance is Context)
                NativeInterface.Setup();
            // In case this is RefCounted object add a reference for duration of object's lifetime.
            (instance as RefCounted)?.AddRef();
            _cache[instance.NativeInstance] = new CacheEntry(instance);
        }

        public static void Remove(IntPtr instance)
        {
            ExpireCache();
            CacheEntry entry;
            _cache.TryRemove(instance, out entry);
            var target = entry.Target;
            if (target == null)
                return;

            if (target is Context)
                NativeInterface.Dispose();

            if (entry.Target is RefCounted)
                ((RefCounted) target).ReleaseRef();
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
                _expirationEnumerator = _cache.GetEnumerator();
                _needsReset = false;
            }

            if (_expirationEnumerator.MoveNext())
            {
                var entry = _expirationEnumerator.Current;
                if (entry.Value.Expired)
                    Remove(entry.Value.Target.NativeInstance);
            }
            else
                _needsReset = true;
        }
    }
}
