using System;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    public static class NativeInterface
    {
        /// <summary>
        /// Lock object preventing garbage collector from freeing it.
        /// </summary>
        /// <param name="object">Object to lock.</param>
        /// <param name="pin">Pass `true` to prevent GC from moving object.</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr LockDelegate(Object @object, bool pin);
        /// <summary>
        /// Unlock previously locked object and make it eligible for garbage collection if no other locks exist.
        /// </summary>
        /// <param name="handle">Pointer returned by Lock().</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void UnlockDelegate(IntPtr handle);
        /// <summary>
        /// Make a duplicate of GC handle. Both handles will have to be unlocked by calling Unlock(handle).
        /// </summary>
        /// <param name="handle">Result of Lock().</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr CloneHandleDelegate(IntPtr handle);
        /// <summary>
        /// Create instance of managed object and return it's native instance pointer.
        /// </summary>
        /// <param name="context">Native instance of Context.</param>
        /// <param name="managedType">StringHash indicating managed type.</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr CreateObjectDelegate(IntPtr context, uint managedType);
        /// <summary>
        /// Calls managed event handler.
        /// </summary>
        /// <param name="gcHandle">Handle of receiver object.</param>
        /// <param name="type">Event type hash.</param>
        /// <param name="args">Instance of VariantMap.</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HandleEventDelegate(IntPtr gcHandle, uint type, IntPtr args);

        [StructLayout(LayoutKind.Sequential)]
        internal struct ManagedRuntime
        {
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public LockDelegate Lock;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public UnlockDelegate Unlock;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public CloneHandleDelegate CloneHandle;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public CreateObjectDelegate CreateObject;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public HandleEventDelegate HandleEventWithType;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public HandleEventDelegate HandleEventWithoutType;
        }

        /// <summary>
        /// Allocates unmanaged memory using malloc().
        /// </summary>
        /// <param name="size">Number of bytes to return.</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr AllocateMemoryDelegate(int size);
        /// <summary>
        /// Free memory allocated by AllocateMemory().
        /// </summary>
        /// <param name="memory">Result of AllocateMemory().</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void FreeMemoryDelegate(IntPtr memory);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr InteropAllocDelegate(int length);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void InteropFreeDelegate(IntPtr ptr);

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeRuntime
        {
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public AllocateMemoryDelegate AllocateMemory;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public FreeMemoryDelegate FreeMemory;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public InteropAllocDelegate InteropAlloc;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public InteropFreeDelegate InteropFree;
        }

        internal static ManagedRuntime Managed;
        internal static NativeRuntime Native;

        internal static void Initialize()
        {
            Managed = new ManagedRuntime();
            Native = new NativeRuntime();

            Managed.Lock = Lock;
            Managed.Unlock = Unlock;
            Managed.CloneHandle = CloneHandle;
            Managed.CreateObject = CreateObject;
            Managed.HandleEventWithType = Object.HandleEventWithType;
            Managed.HandleEventWithoutType = Object.HandleEventWithoutType;

            Urho3D_InitializeCSharp(ref Managed, ref Native);
        }

        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_InitializeCSharp(ref ManagedRuntime managed, ref NativeRuntime native);

        internal static IntPtr Lock(Object @object, bool pin = false)
        {
            var handle = GCHandle.Alloc(@object, pin ? GCHandleType.Pinned : GCHandleType.Normal);
            return GCHandle.ToIntPtr(handle);
        }

        internal static void Unlock(IntPtr handle)
        {
            GCHandle.FromIntPtr(handle).Free();
        }

        internal static IntPtr CloneHandle(IntPtr handle)
        {
            return GCHandle.ToIntPtr(GCHandle.Alloc(GCHandle.FromIntPtr(handle).Target));
        }

        internal static IntPtr CreateObject(IntPtr contextPtr, uint managedType)
        {
            var context = Context.GetManagedInstance(contextPtr, true);
            return context.CreateObject(managedType);
        }

        internal static void Dispose()
        {
            InstanceCache.Dispose();
        }
    }
}
