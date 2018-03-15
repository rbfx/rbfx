using System;
using System.Runtime.InteropServices;
using System.Threading;
using Urho3D;

namespace CSharp
{
    internal delegate void CSharp_FreeGCHandle(IntPtr handle);
    internal delegate IntPtr CSharp_CloneGCHandle(IntPtr handle);
    internal delegate IntPtr CSharp_CreateObject(IntPtr context, uint managedType);

    [StructLayout(LayoutKind.Sequential)]
    struct ManagedInterface
    {
        [MarshalAs(UnmanagedType.FunctionPtr)]
        internal CSharp_FreeGCHandle FreeGcHandle;
        [MarshalAs(UnmanagedType.FunctionPtr)]
        internal CSharp_CloneGCHandle CloneGcHandle;
        [MarshalAs(UnmanagedType.FunctionPtr)]
        internal CSharp_CreateObject CreateObject;
    }

    public class NativeObject : IEquatable<NativeObject>
    {
        internal IntPtr instance_;
        protected volatile int disposed_;

        internal NativeObject(IntPtr instance)
        {
            if (instance != IntPtr.Zero)
                instance_ = instance;
        }

        internal NativeObject()
        {
        }

        internal virtual void SetupInstance(IntPtr instance)
        {
            // Derived types will put object initialization code here. At the time of writing such code sets up virtual
            // method callbacks and nothing more.
        }

        public bool Equals(NativeObject other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return instance_.Equals(other.instance_) && disposed_ == other.disposed_;
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
                return (instance_.GetHashCode() * 397) ^ disposed_;
            }
        }
    }

    public static class NativeInterface
    {
        static ManagedInterface _api;
        static volatile int count_;

        internal static void Setup()
        {
            if (Interlocked.Increment(ref count_) != 1)
                return;

            _api.FreeGcHandle = ptr =>
            {
                var handle = GCHandle.FromIntPtr(ptr);
                (handle.Target as IDisposable)?.Dispose(); // Just in case
                handle.Free();
            };
            _api.CloneGcHandle = ptr =>
            {
                var handle = GCHandle.FromIntPtr(ptr);
                return GCHandle.ToIntPtr(GCHandle.Alloc(handle.Target));
            };
            _api.CreateObject = (contextPtr, managedType) =>
            {
                var context = Context.__FromPInvoke(contextPtr);
                return context.CreateObject(managedType);
            };
            CSharp_SetManagedAPI(_api);
        }

        internal static void Dispose()
        {
            if (Interlocked.Decrement(ref count_) != 0)
                return;

            InstanceCache.Dispose();
        }

        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void CSharp_SetManagedAPI(ManagedInterface netApi);
    }
}
