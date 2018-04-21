using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Urho3D.CSharp;

namespace Urho3D
{
    internal delegate void EventHandler(IntPtr gcHandle, uint type, IntPtr args);

    public partial class Object
    {
        public void SubscribeToEvent(StringHash eventType, Action<StringHash, VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(__ToPInvoke(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, Marshal.GetFunctionPointerForDelegate((EventHandler)HandleEventWithType), IntPtr.Zero);
        }

        public void SubscribeToEvent(StringHash eventType, Action<VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(__ToPInvoke(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, Marshal.GetFunctionPointerForDelegate((EventHandler)HandleEventWithoutType), IntPtr.Zero);
        }

        public void SubscribeToEvent(Object sender, StringHash eventType, Action<StringHash, VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(__ToPInvoke(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, Marshal.GetFunctionPointerForDelegate((EventHandler)HandleEventWithType), __ToPInvoke(sender));
        }

        public void SubscribeToEvent(Object sender, StringHash eventType, Action<VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(__ToPInvoke(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, Marshal.GetFunctionPointerForDelegate((EventHandler)HandleEventWithoutType), __ToPInvoke(sender));
        }

        private void HandleEventWithType(IntPtr gcHandle, uint type, IntPtr args)
        {
            var callback = (Action<StringHash, VariantMap>) GCHandle.FromIntPtr(gcHandle).Target;
            callback.Invoke(StringHash.__FromPInvoke(type), VariantMap.__FromPInvoke(args, false));
        }

        private void HandleEventWithoutType(IntPtr gcHandle, uint type, IntPtr args)
        {
            var callback = (Action<VariantMap>) GCHandle.FromIntPtr(gcHandle).Target;
            callback.Invoke(VariantMap.__FromPInvoke(args, false));
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Urho3D_Object_SubscribeToEvent(IntPtr receiver, IntPtr gcHandle, uint eventType,
            IntPtr function, IntPtr sender);
    }
}
