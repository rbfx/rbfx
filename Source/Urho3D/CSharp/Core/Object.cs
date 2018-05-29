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
            Urho3D_Object_SubscribeToEvent(GetNativeInstance(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, true, IntPtr.Zero);
        }

        public void SubscribeToEvent(StringHash eventType, Action<VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(GetNativeInstance(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, false, IntPtr.Zero);
        }

        public void SubscribeToEvent(Object sender, StringHash eventType, Action<StringHash, VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(GetNativeInstance(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, true, GetNativeInstance(sender));
        }

        public void SubscribeToEvent(Object sender, StringHash eventType, Action<VariantMap> function)
        {
            Urho3D_Object_SubscribeToEvent(GetNativeInstance(this), GCHandle.ToIntPtr(GCHandle.Alloc(function)),
                eventType.Hash, false, GetNativeInstance(sender));
        }

        private static unsafe void HandleEventWithType(void* gcHandle, uint type, void* args)
        {
            var callback = (Action<StringHash, VariantMap>) GCHandle.FromIntPtr((IntPtr)gcHandle).Target;
            callback.Invoke(StringHash.GetManagedInstance(type), VariantMap.GetManagedInstance((IntPtr)args, false));
        }

        private static unsafe void HandleEventWithoutType(void* gcHandle, uint type, void* args)
        {
            var callback = (Action<VariantMap>) GCHandle.FromIntPtr((IntPtr)gcHandle).Target;
            callback.Invoke(VariantMap.GetManagedInstance((IntPtr)args, false));
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Urho3D_Object_SubscribeToEvent(IntPtr receiver, IntPtr gcHandle, uint eventType,
            bool handleWithType, IntPtr sender);
    }
}
