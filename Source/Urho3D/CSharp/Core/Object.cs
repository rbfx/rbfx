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
using System.Collections.Generic;
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

    public struct Event
    {
        private IntPtr _map;

        internal Event(StringHash eventType, IntPtr variantMap)
        {
            EventType = eventType;
            _map = variantMap;
        }

        public StringHash EventType { get; private set; }

        public int GetInt32(StringHash key, int @default=0)
        {
            Urho3D_Object_Event_GetInt32(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetInt32(StringHash key, int value)
        {
            Urho3D_Object_Event_SetInt32(_map, key.Hash, value);
        }

        public bool GetBool(StringHash key, bool @default=false)
        {
            Urho3D_Object_Event_GetBool(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetBool(StringHash key, bool value)
        {
            Urho3D_Object_Event_SetBool(_map, key.Hash, value);
        }

        public float GetFloat(StringHash key, float @default=0)
        {
            Urho3D_Object_Event_GetFloat(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetFloat(StringHash key, float value)
        {
            Urho3D_Object_Event_SetFloat(_map, key.Hash, value);
        }

        public double GetDouble(StringHash key, double @default=0)
        {
            Urho3D_Object_Event_GetDouble(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetDouble(StringHash key, double value)
        {
            Urho3D_Object_Event_SetDouble(_map, key.Hash, value);
        }

        public long GetInt64(StringHash key, long @default=0)
        {
            Urho3D_Object_Event_GetInt64(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetInt64(StringHash key, long value)
        {
            Urho3D_Object_Event_SetInt64(_map, key.Hash, value);
        }

        public object GetObject(StringHash key, object @default=null)
        {
            IntPtr result = IntPtr.Zero;
            var type = 0;
            Urho3D_Object_Event_GetObject(_map, key.Hash, ref type, ref result);
            if (result == IntPtr.Zero)
                return @default;

            if (type == 1)
                // Managed object
                @default = GCHandle.FromIntPtr(result).Target;
            else if (type == 2)
                // Native object
                @default = NativeObject.GetManagedInstanceGeneric<RefCounted>(result);
            else
                throw new Exception();

            return @default;
        }

        public void SetObject(StringHash key, object value)
        {
            if (value is RefCounted obj)
                Urho3D_Object_Event_SetObject(_map, key.Hash, 2, obj.NativeInstance);
            else
                Urho3D_Object_Event_SetObject(_map, key.Hash, 1, GCHandle.ToIntPtr(GCHandle.Alloc(value)));
        }

        public Vector2 GetVector2(StringHash key, Vector2? @default=null)
        {
            var result = @default.GetValueOrDefault(Vector2.Zero);
            Urho3D_Object_Event_GetVector2(_map, key.Hash, ref result);
            return result;
        }

        public void SetVector2(StringHash key, Vector2 value)
        {
            Urho3D_Object_Event_SetVector2(_map, key.Hash, ref value);
        }

        public Vector3 GetVector3(StringHash key, Vector3? @default=null)
        {
            var result = @default.GetValueOrDefault(Vector3.Zero);
            Urho3D_Object_Event_GetVector3(_map, key.Hash, ref result);
            return result;
        }

        public void SetVector3(StringHash key, Vector3 value)
        {
            Urho3D_Object_Event_SetVector3(_map, key.Hash, ref value);
        }

        public Vector4 GetVector4(StringHash key, Vector4? @default=null)
        {
            var result = @default.GetValueOrDefault(Vector4.Zero);
            Urho3D_Object_Event_GetVector4(_map, key.Hash, ref result);
            return result;
        }

        public void SetVector4(StringHash key, Vector4 value)
        {
            Urho3D_Object_Event_SetVector4(_map, key.Hash, ref value);
        }

        public Rect GetRect(StringHash key, Rect? @default=null)
        {
            var result = @default.GetValueOrDefault(Rect.Zero);
            Urho3D_Object_Event_GetRect(_map, key.Hash, ref result);
            return result;
        }

        public void SetRect(StringHash key, Rect value)
        {
            Urho3D_Object_Event_SetRect(_map, key.Hash, ref value);
        }

        public IntVector2 GetIntVector2(StringHash key, IntVector2? @default=null)
        {
            var result = @default.GetValueOrDefault(IntVector2.Zero);
            Urho3D_Object_Event_GetIntVector2(_map, key.Hash, ref result);
            return result;
        }

        public void SetIntVector2(StringHash key, IntVector2 value)
        {
            Urho3D_Object_Event_SetIntVector2(_map, key.Hash, ref value);
        }

        public IntVector3 GetIntVector3(StringHash key, IntVector3? @default=null)
        {
            var result = @default.GetValueOrDefault(IntVector3.Zero);
            Urho3D_Object_Event_GetIntVector3(_map, key.Hash, ref result);
            return result;
        }

        public void SetIntVector3(StringHash key, IntVector3 value)
        {
            Urho3D_Object_Event_SetIntVector3(_map, key.Hash, ref value);
        }

        public IntRect GetIntRect(StringHash key, IntRect? @default=null)
        {
            var result = @default.GetValueOrDefault(IntRect.Zero);
            Urho3D_Object_Event_GetIntRect(_map, key.Hash, ref result);
            return result;
        }

        public void SetIntRect(StringHash key, IntRect value)
        {
            Urho3D_Object_Event_SetIntRect(_map, key.Hash, ref value);
        }

        public Matrix3 GetMatrix3(StringHash key, Matrix3? @default=null)
        {
            var result = @default.GetValueOrDefault(Matrix3.Zero);
            Urho3D_Object_Event_GetMatrix3(_map, key.Hash, ref result);
            return result;
        }

        public void SetMatrix3(StringHash key, Matrix3 value)
        {
            Urho3D_Object_Event_SetMatrix3(_map, key.Hash, ref value);
        }

        public Matrix3x4 GetMatrix3x4(StringHash key, Matrix3x4? @default=null)
        {
            var result = @default.GetValueOrDefault(Matrix3x4.Zero);
            Urho3D_Object_Event_GetMatrix3x4(_map, key.Hash, ref result);
            return result;
        }

        public void SetMatrix3x4(StringHash key, Matrix3x4 value)
        {
            Urho3D_Object_Event_SetMatrix3x4(_map, key.Hash, ref value);
        }

        public Matrix4 GetMatrix4(StringHash key, Matrix4? @default=null)
        {
            var result = @default.GetValueOrDefault(Matrix4.Zero);
            Urho3D_Object_Event_GetMatrix4(_map, key.Hash, ref result);
            return result;
        }

        public void SetMatrix4(StringHash key, Matrix4 value)
        {
            Urho3D_Object_Event_SetMatrix4(_map, key.Hash, ref value);
        }

        public Quaternion GetQuaternion(StringHash key, Quaternion? @default=null)
        {
            var result = @default.GetValueOrDefault(Quaternion.Identity);
            Urho3D_Object_Event_GetQuaternion(_map, key.Hash, ref result);
            return result;
        }

        public void SetQuaternion(StringHash key, Quaternion value)
        {
            Urho3D_Object_Event_SetQuaternion(_map, key.Hash, ref value);
        }

        public Color GetColor(StringHash key, Color? @default=null)
        {
            var result = @default.GetValueOrDefault(Color.Transparent);
            Urho3D_Object_Event_GetColor(_map, key.Hash, ref result);
            return result;
        }

        public void SetColor(StringHash key, Color value)
        {
            Urho3D_Object_Event_SetColor(_map, key.Hash, ref value);
        }

        public string GetString(StringHash key, string @default=null)
        {
            Urho3D_Object_Event_GetString(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetString(StringHash key, string value)
        {
            Urho3D_Object_Event_SetString(_map, key.Hash, value);
        }

        public string[] GetStrings(StringHash key, string[] @default=null)
        {
            Urho3D_Object_Event_GetStrings(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetStrings(StringHash key, string[] value)
        {
            Urho3D_Object_Event_SetStrings(_map, key.Hash, value);
        }

        public byte[] GetBuffer(StringHash key, byte[] @default=null)
        {
            Urho3D_Object_Event_GetBuffer(_map, key.Hash, ref @default);
            return @default;
        }

        public void SetBuffer(StringHash key, byte[] value)
        {
            Urho3D_Object_Event_SetBuffer(_map, key.Hash, value);
        }

        public ResourceRef GetResourceRef(StringHash key, ResourceRef @default=null)
        {
            IntPtr instance = IntPtr.Zero;
            Urho3D_Object_Event_GetResourceRef(_map, key.Hash, ref instance);
            if (instance == IntPtr.Zero)
                return @default;
            return ResourceRef.GetManagedInstance(instance);
        }

        public void SetResourceRef(StringHash key, ResourceRef value)
        {
            Urho3D_Object_Event_SetResourceRef(_map, key.Hash, value.NativeInstance);
        }

        public ResourceRefList GetResourceRefList(StringHash key, ResourceRefList @default=null)
        {
            IntPtr instance = IntPtr.Zero;
            Urho3D_Object_Event_GetResourceRefList(_map, key.Hash, ref instance);
            if (instance == IntPtr.Zero)
                return @default;
            return ResourceRefList.GetManagedInstance(instance);
        }

        public void SetResourceRefList(StringHash key, ResourceRefList value)
        {
            Urho3D_Object_Event_SetResourceRefList(_map, key.Hash, value.NativeInstance);
        }

        #region Interop
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetInt32(IntPtr map, uint key, ref int value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetInt32(IntPtr map, uint key, int value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetBool(IntPtr map, uint key, ref bool value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetBool(IntPtr map, uint key, bool value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetFloat(IntPtr map, uint key, ref float value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetFloat(IntPtr map, uint key, float value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetDouble(IntPtr map, uint key, ref double value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetDouble(IntPtr map, uint key, double value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetInt64(IntPtr map, uint key, ref long value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetInt64(IntPtr map, uint key, long value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetObject(IntPtr map, uint key, ref int type, ref IntPtr value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetObject(IntPtr map, uint key, int type, IntPtr value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetVector2(IntPtr map, uint key, ref Vector2 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetVector2(IntPtr map, uint key, ref Vector2 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetVector3(IntPtr map, uint key, ref Vector3 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetVector3(IntPtr map, uint key, ref Vector3 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetVector4(IntPtr map, uint key, ref Vector4 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetVector4(IntPtr map, uint key, ref Vector4 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetRect(IntPtr map, uint key, ref Rect value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetRect(IntPtr map, uint key, ref Rect value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetIntVector2(IntPtr map, uint key, ref IntVector2 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetIntVector2(IntPtr map, uint key, ref IntVector2 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetIntVector3(IntPtr map, uint key, ref IntVector3 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetIntVector3(IntPtr map, uint key, ref IntVector3 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetIntRect(IntPtr map, uint key, ref IntRect value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetIntRect(IntPtr map, uint key, ref IntRect value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetMatrix3(IntPtr map, uint key, ref Matrix3 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetMatrix3(IntPtr map, uint key, ref Matrix3 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetMatrix3x4(IntPtr map, uint key, ref Matrix3x4 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetMatrix3x4(IntPtr map, uint key, ref Matrix3x4 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetMatrix4(IntPtr map, uint key, ref Matrix4 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetMatrix4(IntPtr map, uint key, ref Matrix4 value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetQuaternion(IntPtr map, uint key, ref Quaternion value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetQuaternion(IntPtr map, uint key, ref Quaternion value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetColor(IntPtr map, uint key, ref Color value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetColor(IntPtr map, uint key, ref Color value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetString(IntPtr map, uint key, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]ref string value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetString(IntPtr map, uint key, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetStrings(IntPtr map, uint key, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8ArrayMarshaller))]ref string[] value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetStrings(IntPtr map, uint key, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8ArrayMarshaller))]string[] value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetBuffer(IntPtr map, uint key, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(PodArrayMarshaller<byte>))]ref byte[] value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetBuffer(IntPtr map, uint key, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(PodArrayMarshaller<byte>))]byte[] value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetResourceRef(IntPtr map, uint key, ref IntPtr value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetResourceRef(IntPtr map, uint key, IntPtr value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_GetResourceRefList(IntPtr map, uint key, ref IntPtr value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_Event_SetResourceRefList(IntPtr map, uint key, IntPtr value);
        #endregion
    }

    public partial class Object
    {
        private static VariantMap _args = new VariantMap();

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

        public void SubscribeToEvent(Object sender, StringHash eventType, Action<Event> handler)
        {
            Urho3D_Object_SubscribeToEvent(NativeInstance, GCHandle.ToIntPtr(GCHandle.Alloc(handler)), eventType.Hash,
                sender?.NativeInstance ?? IntPtr.Zero);
        }

        public void SubscribeToEvent(StringHash eventType, Action<Event> handler)
        {
            SubscribeToEvent(null, eventType, handler);
        }

        public void SubscribeToEvent<T>(Object sender, Action<Event> handler)
        {
            SubscribeToEvent(sender, GetEventType<T>(), handler);
        }

        public void SubscribeToEvent<T>(Action<Event> handler)
        {
            SubscribeToEvent(null, GetEventType<T>(), handler);
        }

        public void SendEvent(StringHash eventType, IDictionary<StringHash, dynamic> args)
        {
            _args.Clear();
            if (args != null)
            {
                foreach (var pair in args)
                    _args[pair.Key] = pair.Value;
                args.Clear();
            }
            SendEvent(eventType, _args);
        }

        public void SendEvent<T>()
        {
            SendEvent(GetEventType<T>());
        }

        public void SendEvent<T>(IDictionary<StringHash, dynamic> args)
        {
            SendEvent(GetEventType<T>(), args);
        }

        internal static void HandleEvent(IntPtr gcHandle, uint type, IntPtr args)
        {
            var callback = (Action<Event>) GCHandle.FromIntPtr(gcHandle).Target;
            callback.Invoke(new Event(new StringHash(type), args));
        }

        public T GetSubsystem<T>() where T: Object
        {
            return (T) Context.GetSubsystem(new StringHash(typeof(T).Name));
        }

        #region Interop
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Object_SubscribeToEvent(IntPtr receiver, IntPtr gcHandle, uint eventType,
            IntPtr sender);
        #endregion
    }
}
