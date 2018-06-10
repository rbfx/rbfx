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
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Security;
using Urho3D.CSharp;

namespace Urho3D
{
    internal delegate void EventHandler(IntPtr gcHandle, uint type, IntPtr args);

    public class EventAttribute : System.Attribute
    {
        public string EventName
        {
            get => EventHash.ToString();
            set => EventHash = new StringHash(value);
        }

        public uint EventType
        {
            get => EventHash.Hash;
            set => EventHash = new StringHash(value);
        }

        public StringHash EventHash { get; set; }
    }

    public partial class Object
    {
        private StringHash GetEventType<T>()
        {
            var type = typeof(T);
            var attribute = (EventAttribute) System.Attribute.GetCustomAttribute(type, typeof(EventAttribute));
            if (attribute == null)
                throw new CustomAttributeFormatException("Specified type does not have [EventAttribute].");
            if (attribute.EventType == 0)
                return new StringHash(type.Name);
            return attribute.EventHash;
        }

        public void SendEvent<T>()
        {
            SendEvent(GetEventType<T>());
        }

        public void SendEvent<T>(VariantMap args)
        {
            SendEvent(GetEventType<T>(), args);
        }

        public void SubscribeToEvent<T>(Action<StringHash, VariantMap> function)
        {
            SubscribeToEvent(GetEventType<T>(), function);
        }

        public void SubscribeToEvent<T>(Action<VariantMap> function)
        {
            SubscribeToEvent(GetEventType<T>(), function);
        }

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

        internal static void HandleEventWithType(IntPtr gcHandle, uint type, IntPtr args)
        {
            var callback = (Action<StringHash, VariantMap>) GCHandle.FromIntPtr(gcHandle).Target;
            callback.Invoke(StringHash.GetManagedInstance(type), VariantMap.GetManagedInstance(args, false));
        }

        internal static void HandleEventWithoutType(IntPtr gcHandle, uint type, IntPtr args)
        {
            var callback = (Action<VariantMap>) GCHandle.FromIntPtr(gcHandle).Target;
            callback.Invoke(VariantMap.GetManagedInstance(args, false));
        }

        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_SubscribeToEvent(IntPtr receiver, IntPtr gcHandle, uint eventType,
            bool handleWithType, IntPtr sender);
    }
}
