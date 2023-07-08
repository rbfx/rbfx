//
// Copyright (c) 2023-2023 the rbfx project.
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
