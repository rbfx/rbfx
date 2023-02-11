//
// Copyright (c) 2021-2022 the rbfx project.
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
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class ActionBuilder
    {
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_ActionBuilder_CallFunc")]
        private static extern IntPtr Urho3D_ActionBuilder_CallFunc(HandleRef receiver, IntPtr callback, IntPtr callbackHandle);


#if __IOS__
        [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
        private delegate void ActionCallbackDelegate(IntPtr actionHandle, IntPtr obj);

#if __IOS__
        [global::ObjCRuntime.MonoPInvokeCallback(typeof(ActionCallbackDelegate))]
#endif
        private static void ActionHandlerCallback(IntPtr actionHandle, IntPtr obj)
        {
            var eventHandler = (Action<Object>)GCHandle.FromIntPtr(actionHandle).Target;
            eventHandler(Object.wrap(obj, false));
        }
        private static readonly ActionCallbackDelegate EventHandlerCallbackInstance = ActionHandlerCallback;


        public ActionBuilder CallFunc(Action<Object> eventHandler)
        {
            IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(eventHandler));
            IntPtr callback = Marshal.GetFunctionPointerForDelegate(EventHandlerCallbackInstance);
            return ActionBuilder.wrap(Urho3D_ActionBuilder_CallFunc(swigCPtr, callback, handle), true);
        }
    }
}
