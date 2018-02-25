using System;
using System.Collections.Concurrent;

namespace CSharp
{
    static class InstanceCache
    {
        internal static ConcurrentDictionary<Type, WeakDictionary<IntPtr, object>> cache_ =
            new ConcurrentDictionary<Type, WeakDictionary<IntPtr, object>>();

        private static WeakDictionary<IntPtr, object> GetContainer<T>()
        {
            return cache_.GetOrAdd(typeof(T), t => new WeakDictionary<IntPtr, object>());
        }

        public static T GetOrAdd<T>(IntPtr instance, Func<IntPtr, T> factory)
        {
            return (T)GetContainer<T>().GetOrAdd(instance, ptr => (object)factory(ptr));
        }

        public static void Add<T>(IntPtr instance, T object_)
        {
            GetContainer<T>().Add(instance, object_);
        }

        public static void Remove<T>(IntPtr instance)
        {
            GetContainer<T>().Remove(instance);
        }

        /// <summary>
        /// Dispose all cached references. Should be called on application exit before disposing Context.
        /// </summary>
        public static void Dispose()
        {
            foreach (var pair in cache_)
            {
                foreach (var item in pair.Value)
                {
                    ((IDisposable)item.Value.Target)?.Dispose();
                }
            }
            cache_.Clear();
        }
    }
}
