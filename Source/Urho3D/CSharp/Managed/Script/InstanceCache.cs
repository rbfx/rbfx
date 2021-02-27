//
// Copyright (c) 2017-2020 the rbfx project.
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
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    internal class InstanceCache<T> : IDisposable where T : class
    {
        /// <summary>
        /// How long (ms) strong reference is kept in CacheEntry after last access.
        /// </summary>
        private const int StrongRefExpirationTime = 10000;
        /// <summary>
        /// One iteration through entire cache is allowed during this interval (ms).
        /// </summary>
        private const int CacheIterationInterval = 30000;

        internal struct CacheEntry
        {
            // Strong reference is kept for some time after last object access and then set to null. This prevents
            // garbage-collection of often used objects whose reference is not kept around. When weak reference expires
            // entry is removed from cache.

            private T _target;
            private readonly WeakReference<T> _targetWeak;
            public int LastAccess;

            public T Target
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

            public CacheEntry(T cachedTarget)
            {
                _target = cachedTarget;
                _targetWeak = new WeakReference<T>(cachedTarget);
                LastAccess = Environment.TickCount;
            }

            internal bool IsExpired
            {
                get
                {
                    if (_target != null && Environment.TickCount - LastAccess > StrongRefExpirationTime)
                        _target = null;
                    return !_targetWeak.TryGetTarget(out _);
                }
            }
        }

        private Dictionary<IntPtr, CacheEntry> _cache = new Dictionary<IntPtr, CacheEntry>();

        public T GetOrAdd(IntPtr instance, Func<T> factory)
        {
            lock (_cache)
            {
                if (!_cache.TryGetValue(instance, out var entry))
                {
                    entry = new CacheEntry(factory());
                    _needsReset = true;
                    _cache[instance] = entry;
                }

                entry.LastAccess = Environment.TickCount;
                ExpireCache();
                return entry.Target;
            }
        }

        public void AddNew(HandleRef instance)
        {
            lock (_cache)
            {
                ExpireCache();
                if (!_cache.ContainsKey(instance.Handle))
                {
                    _needsReset = true;
                    _cache[instance.Handle] = new CacheEntry((T) instance.Wrapper);
                }
            }
        }

        public void Remove(HandleRef instance)
        {
            lock (_cache)
            {
                _cache.Remove(instance.Handle);
                _needsReset = true;
            }
        }

        public void Remove(IntPtr instance)
        {
            lock (_cache)
            {
                _cache.Remove(instance);
                _needsReset = true;
            }
        }

        /// <summary>
        /// Dispose all cached references. Should be called on application exit before disposing Context.
        /// </summary>
        public void Dispose()
        {
            lock (_cache)
            {
                _needsReset = true;
                _expirationEnumerator = null;
                _lastCacheEnumeratorResetTime = Environment.TickCount;
                foreach (var value in _cache.Values.ToArray())
                {
                    if (!(value.Target is Context))    // Context must be destroyed at very end.
                        ((IDisposable) value.Target)?.Dispose();
                }
            }
        }

        private int _lastCacheEnumeratorResetTime = 0;
        private bool _needsReset = true;
        private IEnumerator<KeyValuePair<IntPtr, CacheEntry>> _expirationEnumerator;

        private void ExpireCache()
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
                if (entry.Value.IsExpired)
                    Remove(entry.Key);
            }
            else
                _needsReset = true;
        }
    }
}
