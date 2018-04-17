using System;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    public interface INativeObject : IDisposable
    {
        IntPtr NativeInstance { get; }
        bool IsDisposed { get; }
    }

    public abstract class NativeObject : INativeObject, IEquatable<NativeObject>
    {
        public IntPtr NativeInstance { get; protected set; } = IntPtr.Zero;
        protected bool OwnsNativeInstance { get; set; }
        public bool IsDisposed => DisposedCounter == 0;
        protected volatile int DisposedCounter = 0;

        internal NativeObject(IntPtr instance, bool ownsInstance=false)
        {
            if (instance != IntPtr.Zero)
            {
                NativeInstance = instance;
                OwnsNativeInstance = ownsInstance;
            }
        }

        internal NativeObject()
        {
        }

        ~NativeObject()
        {
            Dispose();
        }

        // Derived types will put object initialization code here. At the time of writing such code sets up virtual
        // method callbacks and nothing more.
        internal virtual void PerformInstanceSetup(IntPtr instance, bool ownsInstance)
        {
            OwnsNativeInstance = ownsInstance;
            NativeInstance = instance;
            InstanceCache.Add(this);
        }

        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal virtual void SetupInstance(IntPtr instance, bool ownsInstance)
        {
            PerformInstanceSetup(instance, ownsInstance);
        }

        public bool Equals(NativeObject other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return NativeInstance.Equals(other.NativeInstance) && IsDisposed == other.IsDisposed;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((NativeObject) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (NativeInstance.GetHashCode() * 397) ^ DisposedCounter;
            }
        }

        public virtual void Dispose()
        {
        }
    }

    public static class NativeInterface
    {
        internal static void FreeGcHandle(IntPtr ptr)
        {
            var handle = GCHandle.FromIntPtr(ptr);
            (handle.Target as IDisposable)?.Dispose(); // Just in case
            handle.Free();
        }

        internal static IntPtr CloneGcHandle(IntPtr ptr)
        {
            var handle = GCHandle.FromIntPtr(ptr);
            return GCHandle.ToIntPtr(GCHandle.Alloc(handle.Target));
        }

        internal static IntPtr CreateObject(IntPtr contextPtr, uint managedType)
        {
            var context = Context.__FromPInvoke(contextPtr, true);
            return context.CreateObject(managedType);
        }

        internal static void Dispose()
        {
            InstanceCache.Dispose();
        }
    }
}
