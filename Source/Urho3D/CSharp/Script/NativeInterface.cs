//
// Copyright (c) 2018 Rokas Kupstys
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
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;

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

        /// <summary>
        /// Invokes delegate passing it specified number of parameters.
        /// </summary>
        /// <param name="methodInfo">GCHandle to MethodInfo.</param>
        /// <param name="paramCount">Number of items in `parameters` array.</param>
        /// <param name="parameters">Array of parameters. May be null.</param>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public unsafe delegate IntPtr InvokeMethodDelegate(IntPtr methodInfo, int paramCount, void** parameters);

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
            public HandleEventDelegate HandleEvent;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            public InvokeMethodDelegate InvokeMethod;
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

        internal static unsafe void Initialize()
        {
            Managed = new ManagedRuntime();
            Native = new NativeRuntime();

            Managed.Lock = Lock;
            Managed.Unlock = Unlock;
            Managed.CloneHandle = CloneHandle;
            Managed.CreateObject = CreateObject;
            Managed.HandleEvent = Object.HandleEvent;
            Managed.InvokeMethod = InvokeMethod;

            Urho3D_InitializeCSharp(ref Managed, ref Native);
        }

        [SuppressUnmanagedCodeSecurity]
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
            var context = Context.GetManagedInstance(contextPtr);
            return context.CreateObject(managedType);
        }

        internal static unsafe IntPtr InvokeMethod(IntPtr methodInfoHandle, int paramCount, void** parameters)
        {
            var methodInfo = (MethodInfo)GCHandle.FromIntPtr(methodInfoHandle).Target;
            object targetObject = null;
            if (paramCount > 0)
            {
                var objectHandle = (IntPtr) parameters[0];
                if (objectHandle != IntPtr.Zero)
                    targetObject = GCHandle.FromIntPtr(objectHandle).Target;
            }
            var paramsArray = new object[Math.Max(0, paramCount - 1)];
            for (var i = 1; i < paramCount; i++)
                paramsArray[i - 1] = (IntPtr)parameters[i];
            var result = methodInfo.Invoke(targetObject, paramsArray);
            return (IntPtr?) result ?? IntPtr.Zero;
        }

        internal static void Dispose()
        {
            InstanceCache.Dispose();
        }

        internal static int GetLengthOfAllocatedMemory(IntPtr memory)
        {
            return Marshal.ReadInt32(memory - 4) & 0x7FFFFFFF;
        }

        internal static int StrLen(IntPtr str)
        {
            var length = 0;
            while (Marshal.ReadByte(str, length) != 0)
                length++;
            return length;
        }
    }
}
