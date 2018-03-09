using System;
using System.Runtime.InteropServices;
using System.Threading;


namespace CSharp
{
    internal delegate void CSharp_FreeGCHandle(IntPtr handle);

    [StructLayout(LayoutKind.Sequential)]
    struct ManagedInterface
    {
        [MarshalAs(UnmanagedType.FunctionPtr)]
        internal CSharp_FreeGCHandle FreeGcHandle;
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
                ((IDisposable) handle.Target)?.Dispose(); // Just in case
                handle.Free();
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
