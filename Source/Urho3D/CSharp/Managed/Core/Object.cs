// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class Object
    {
        public static readonly string ClassName = nameof(Object);

        public static readonly StringHash TypeId = new StringHash(nameof(Object));

        public static readonly string BaseClassName = null;

        public static string GetTypeNameStatic() { return ClassName; }

        public static StringHash GetTypeStatic() { return TypeId; }

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
            return (T)GetSubsystem(ObjectReflection<T>.TypeId);
        }
    }
}
