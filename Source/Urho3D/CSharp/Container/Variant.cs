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
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using Urho3D.CSharp;

namespace Urho3D
{
    public partial class Variant
    {
        public Variant(object value)
        {
            var instance = Urho3D_Variant__Variant_object(GCHandle.ToIntPtr(GCHandle.Alloc(value)));
            SetupInstance(instance, true);
        }

        public static Variant FromObject(object value)
        {
            return new Variant(value);
        }

        public object Object
        {
            get
            {
                if (NativeInstance == IntPtr.Zero)
                    throw new ObjectDisposedException("Variant");

                if (Type == VariantType.Ptr)
                {
                    var instance = Urho3D_Variant__GetPtrValue(NativeInstance);
                    if (instance != IntPtr.Zero)
                    {
                        var type = InstanceCache.GetNativeType(RefCounted.GetNativeTypeId(instance));
                        var getManaged = type.GetMethod("GetManagedInstance",
                            BindingFlags.NonPublic | BindingFlags.Static);
                        return getManaged.Invoke(null, new object[] {instance, false});
                    }
                }
                else if (Type == VariantType.CustomStack)
                {
                    var handlePtr = Urho3D_Variant__GetObject(NativeInstance);
                    if (handlePtr != IntPtr.Zero)
                        return GCHandle.FromIntPtr(handlePtr).Target;
                }

                return null;
            }
            set
            {
                if (NativeInstance == IntPtr.Zero)
                    throw new ObjectDisposedException("Variant");

                if (value is RefCounted refCounted)
                {
                    Urho3D_Variant__SetPtrValue(NativeInstance, refCounted.NativeInstance);
                }
                else
                {
                    Urho3D_Variant__SetGcHandleValue(NativeInstance, GCHandle.ToIntPtr(GCHandle.Alloc(value)));
                }
            }
        }

        public VariantType Type
        {
            get
            {
                if (NativeInstance == IntPtr.Zero)
                    throw new ObjectDisposedException("Variant");
                return Urho3D_Variant__GetValueType(NativeInstance);
            }
        }

        #region Interop
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Urho3D_Variant__Variant_object(IntPtr handle);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Urho3D_Variant__GetObject(IntPtr variant);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern VariantType Urho3D_Variant__GetValueType(IntPtr variant);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Urho3D_Variant__GetPtrValue(IntPtr variant);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_Variant__SetPtrValue(IntPtr variant, IntPtr value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_Variant__SetGcHandleValue(IntPtr variant, IntPtr value);
        #endregion
    }
}
