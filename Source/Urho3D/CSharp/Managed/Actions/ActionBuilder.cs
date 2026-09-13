// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
