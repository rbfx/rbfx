//
// Copyright (c) 2017-2020 the rbfx project.
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
    public partial class Object
    {
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_Object_SubscribeToEvent")]
        private static extern void Urho3D_Object_SubscribeToEvent(HandleRef receiver, HandleRef sender, uint eventType,
            IntPtr callback, IntPtr callbackHandle);

#if __IOS__
        [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
        private delegate void EventCallbackDelegate(IntPtr actionHandle, uint eventHash, IntPtr argMap);

#if __IOS__
        [global::ObjCRuntime.MonoPInvokeCallback(typeof(EventCallbackDelegate))]
#endif
        private static void EventHandlerCallback(IntPtr actionHandle, uint eventHash, IntPtr argMap)
        {
            var eventHandler = (Action<StringHash, VariantMap>)GCHandle.FromIntPtr(actionHandle).Target;
            eventHandler(new StringHash(eventHash), VariantMap.wrap(argMap, false));
        }
        private static readonly EventCallbackDelegate EventHandlerCallbackInstance = EventHandlerCallback;

        public void SubscribeToEvent(StringHash e, Object sender, Action<StringHash, VariantMap> eventHandler)
        {
            IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(eventHandler));
            IntPtr callback = Marshal.GetFunctionPointerForDelegate(EventHandlerCallbackInstance);
            Urho3D_Object_SubscribeToEvent(swigCPtr, getCPtr(sender), e.Hash, callback, handle);
        }

        public void SubscribeToEvent(StringHash e, Object sender, Action<VariantMap> eventHandler)
        {
            SubscribeToEvent(e, sender, (evt, args) => eventHandler(args));
        }

        public void SubscribeToEvent(StringHash e, Action<StringHash, VariantMap> eventHandler)
        {
            SubscribeToEvent(e, null, eventHandler);
        }

        public void SubscribeToEvent(StringHash e, Action<VariantMap> eventHandler)
        {
            SubscribeToEvent(e, null, eventHandler);
        }

        public T GetSubsystem<T>() where T : Object
        {
            return (T)GetSubsystem(typeof(T).Name);
        }
    }
}
