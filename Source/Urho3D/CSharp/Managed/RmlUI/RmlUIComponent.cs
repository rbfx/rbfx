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

using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class RmlUIComponent
    {
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_RmlUIComponent_BindDataModelProperty")]
        private static extern bool Urho3D_RmlUIComponent_BindDataModelProperty(HandleRef receiver, string name, IntPtr getter, IntPtr getterHandle, IntPtr setter, IntPtr setterHandle);

        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_RmlUIComponent_BindDataModelEvent")]
        private static extern bool Urho3D_RmlUIComponent_BindDataModelEvent(HandleRef receiver, string name, IntPtr callback, IntPtr callbackHandle);

#if __IOS__
        [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
        private delegate void GetterDelegate(IntPtr actionHandle, IntPtr argPtr);

#if __IOS__
        [global::ObjCRuntime.MonoPInvokeCallback(typeof(GetterDelegate))]
#endif
        private static void GetterCallback(IntPtr actionHandle, IntPtr argPtr)
        {
            var eventHandler = (Action<Variant>)GCHandle.FromIntPtr(actionHandle).Target;
            eventHandler(Variant.wrap(argPtr, false));
        }

#if __IOS__
        [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
        private delegate void SetterDelegate(IntPtr actionHandle, IntPtr argPtr);

#if __IOS__
        [global::ObjCRuntime.MonoPInvokeCallback(typeof(SetterDelegate))]
#endif
        private static void SetterCallback(IntPtr actionHandle, IntPtr argPtr)
        {
            var eventHandler = (Action<Variant>)GCHandle.FromIntPtr(actionHandle).Target;
            eventHandler(Variant.wrap(argPtr, false));
        }

#if __IOS__
        [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
        private delegate void EventDelegate(IntPtr actionHandle, IntPtr argPtr);

#if __IOS__
        [global::ObjCRuntime.MonoPInvokeCallback(typeof(EventDelegate))]
#endif
        private static void EventCallback(IntPtr actionHandle, IntPtr argPtr)
        {
            var eventHandler = (Action<VariantList>)GCHandle.FromIntPtr(actionHandle).Target;
            eventHandler(VariantList.wrap(argPtr, false));
        }

        private static readonly GetterDelegate GetterCallbackInstance = GetterCallback;
        private static readonly SetterDelegate SetterCallbackInstance = SetterCallback;
        private static readonly EventDelegate EventCallbackInstance = EventCallback;

        public bool BindDataModelProperty(string name, Action<Variant> getter, Action<Variant> setter)
        {
            IntPtr getterHandle = GCHandle.ToIntPtr(GCHandle.Alloc(getter));
            IntPtr getterCallback = Marshal.GetFunctionPointerForDelegate(GetterCallbackInstance);

            IntPtr setterHandle = GCHandle.ToIntPtr(GCHandle.Alloc(setter));
            IntPtr setterCallback = Marshal.GetFunctionPointerForDelegate(SetterCallbackInstance);

            return Urho3D_RmlUIComponent_BindDataModelProperty(swigCPtr, name, getterCallback, getterHandle, setterCallback, setterHandle);
        }

        public bool BindDataModelEvent(string name, Action<global::Urho3DNet.VariantList> callback)
        {
            IntPtr callbackHandle = GCHandle.ToIntPtr(GCHandle.Alloc(callback));
            IntPtr callbackPtr = Marshal.GetFunctionPointerForDelegate(EventCallbackInstance);

            return Urho3D_RmlUIComponent_BindDataModelEvent(swigCPtr, name, callbackPtr, callbackHandle);
        }
    }
}
