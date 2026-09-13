// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Runtime.InteropServices;
using System;

namespace Urho3DNet
{
    public partial class WorkQueue
    {
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_WorkQueue_PostTaskForMainThread")]
        private static extern void Urho3D_WorkQueue_PostTaskForMainThread(HandleRef receiver, IntPtr callback, IntPtr callbackHandle, TaskPriority priority);

        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_WorkQueue_PostDelayedTaskForMainThread")]
        private static extern void Urho3D_WorkQueue_PostDelayedTaskForMainThread(HandleRef receiver, IntPtr callback, IntPtr callbackHandle);

#if __IOS__
        [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
        private delegate void PostTaskForMainThreadDelegate(IntPtr actionHandle, uint threadId, IntPtr workQueue);

#if __IOS__
        [global::ObjCRuntime.MonoPInvokeCallback(typeof(PostTaskForMainThreadDelegate))]
#endif
        private static void PostTaskForMainThreadCallback(IntPtr actionHandle, uint threadId, IntPtr workQueue)
        {
            var eventHandler = (Action<uint, WorkQueue>)GCHandle.FromIntPtr(actionHandle).Target;
            eventHandler(threadId, WorkQueue.wrap(workQueue, false));
        }

        private static readonly PostTaskForMainThreadDelegate GetterCallbackInstance = PostTaskForMainThreadCallback;

        public void PostTaskForMainThread(Action<uint, WorkQueue> taskFunction, TaskPriority priority = TaskPriority.Medium)
        {
            IntPtr taskFuncHandle = GCHandle.ToIntPtr(GCHandle.Alloc(taskFunction));
            IntPtr taskFuncCallback = Marshal.GetFunctionPointerForDelegate(GetterCallbackInstance);

            Urho3D_WorkQueue_PostTaskForMainThread(swigCPtr,  taskFuncCallback, taskFuncHandle, priority);
        }

        public void PostDelayedTaskForMainThread(Action<uint, WorkQueue> taskFunction)
        {
            IntPtr taskFuncHandle = GCHandle.ToIntPtr(GCHandle.Alloc(taskFunction));
            IntPtr taskFuncCallback = Marshal.GetFunctionPointerForDelegate(GetterCallbackInstance);

            Urho3D_WorkQueue_PostDelayedTaskForMainThread(swigCPtr, taskFuncCallback, taskFuncHandle);
        }
    }
}
