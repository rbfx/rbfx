using System;
using System.Collections.Concurrent;
using System.Collections.Generic;

namespace Urho3D
{
    public sealed class WeakDictionary<TKey, TValue> {
        private ConcurrentDictionary<TKey, WeakReference> map_;

        public WeakDictionary()
            : this(EqualityComparer<TKey>.Default)
        {
        }

        public WeakDictionary(IEqualityComparer<TKey> comparer)
        {
            map_ = new ConcurrentDictionary<TKey, WeakReference>(comparer);
        }

        public void Add(TKey key, TValue value)
        {
            map_[key] = new WeakReference(value);
        }

        public bool Remove(TKey key)
        {
            WeakReference weakRef;
            return map_.TryRemove(key, out weakRef);
        }

        public TValue GetOrAdd(TKey key, Func<TKey, TValue> factory)
        {
            var weakRef = map_.GetOrAdd(key, theKey => new WeakReference(factory(theKey)));
            var target = (TValue)weakRef.Target;
            if (target == null)
            {
                map_.TryRemove(key, out weakRef);
                target = factory(key);
                Add(key, target);
            }
            return target;
        }
    }
}
