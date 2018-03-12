using System;
using System.Collections.Concurrent;
using Urho3D;

namespace CSharp
{
    static class InstanceCache
    {
        internal static WeakDictionary<IntPtr, IDisposable> cache_ = new WeakDictionary<IntPtr, IDisposable>();

        public static T GetOrAdd<T>(IntPtr instance, Func<IntPtr, T> factory)
        {
            return (T)cache_.GetOrAdd(instance, ptr =>
            {
                var object_ = (IDisposable)factory(ptr);
                // In case this is RefCounted object add a reference for duration of object's lifetime.
                (object_ as RefCounted)?.AddRef();
                return object_;
            });
        }

        public static void Add<T>(IntPtr instance, Context object_)
        {
            NativeInterface.Setup();
            // In case this is RefCounted object add a reference for duration of object's lifetime.
            object_.AddRef();
            cache_.Add(instance, object_);
        }

        public static void Add<T>(IntPtr instance, T object_)
        {
            // In case this is RefCounted object add a reference for duration of object's lifetime.
            (object_ as RefCounted)?.AddRef();
            cache_.Add(instance, (IDisposable)object_);
        }

        public static void Remove<T>(IntPtr native, Context object_)
        {
            cache_.Remove(native);
            NativeInterface.Dispose();
            object_.ReleaseRef();
        }

        public static void Remove<T>(IntPtr native, T object_)
        {
            cache_.Remove(native);
            (object_ as RefCounted)?.ReleaseRef();
        }

        /// <summary>
        /// Dispose all cached references. Should be called on application exit before disposing Context.
        /// </summary>
        public static void Dispose()
        {
            foreach (var item in cache_)
                ((IDisposable) item.Value.Target)?.Dispose();
            cache_.Clear();
        }
    }
}
