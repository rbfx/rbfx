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
            SetupInstance(instance);
        }

        public static Variant FromObject(object value)
        {
            if (value is int typed1)
                return new Variant(typed1);
            if (value is bool typed2)
                return new Variant(typed2);
            if (value is float typed3)
                return new Variant(typed3);
            if (value is Vector2 typed4)
                return new Variant(typed4);
            if (value is Vector3 typed5)
                return new Variant(typed5);
            if (value is Vector4 typed6)
                return new Variant(typed6);
            if (value is Quaternion typed7)
                return new Variant(typed7);
            if (value is Color typed8)
                return new Variant(typed8);
            if (value is string typed9)
                return new Variant(typed9);
            if (value is byte[] typed10)
                return new Variant(typed10);
            if (value is IntPtr typed11)
                return new Variant(typed11);
            if (value is ResourceRef typed12)
                return new Variant(typed12);
            if (value is ResourceRefList typed24)
                return new Variant(typed24);
            //return VariantType.Variantvector;
            //return VariantType.Variantmap;
            if (value is IntRect typed13)
                return new Variant(typed13);
            if (value is IntVector2 typed14)
                return new Variant(typed14);
            if (value is RefCounted typed23)
                return new Variant(typed23);
            if (value is Matrix3 typed15)
                return new Variant(typed15);
            if (value is Matrix3x4 typed16)
                return new Variant(typed16);
            if (value is Matrix4 typed17)
                return new Variant(typed17);
            if (value is Double typed18)
                return new Variant(typed18);
            //if (value is string[] typed19)
            //    return new Variant(typed19);
            if (value is Rect typed20)
                return new Variant(typed20);
            if (value is IntVector3 typed21)
                return new Variant(typed21);
            if (value is long typed22)
                return new Variant(typed22);

            return new Variant(value);
        }

        public static VariantType GetVariantType(Type type)
        {
            if (type == typeof(int))
                return VariantType.Int;
            if (type == typeof(bool))
                return VariantType.Bool;
            if (type == typeof(float))
                return VariantType.Float;
            if (type == typeof(Vector2))
                return VariantType.Vector2;
            if (type == typeof(Vector3))
                return VariantType.Vector3;
            if (type == typeof(Vector4))
                return VariantType.Vector4;
            if (type == typeof(Quaternion))
                return VariantType.Quaternion;
            if (type == typeof(Color))
                return VariantType.Color;
            if (type == typeof(string))
                return VariantType.String;
            if (type == typeof(byte[]))
                return VariantType.Buffer;
            if (type == typeof(IntPtr))
                return VariantType.Voidptr;
            if (type == typeof(ResourceRef))
                return VariantType.Resourceref;
            if (type == typeof(ResourceRefList))
                return VariantType.Resourcereflist;
            //return VariantType.Variantvector;
            //return VariantType.Variantmap;
            if (type == typeof(IntRect))
                return VariantType.Intrect;
            if (type == typeof(IntVector2))
                return VariantType.Intvector2;
            if (type == typeof(RefCounted) || type.IsSubclassOf(typeof(RefCounted)))
                return VariantType.Ptr;
            if (type == typeof(Matrix3))
                return VariantType.Matrix3;
            if (type == typeof(Matrix3x4))
                return VariantType.Matrix3x4;
            if (type == typeof(Matrix4))
                return VariantType.Matrix4;
            if (type == typeof(Double))
                return VariantType.Double;
            //if (type == typeof(string[]))
            //    return VariantType.Stringvector;
            if (type == typeof(Rect))
                return VariantType.Rect;
            if (type == typeof(IntVector3))
                return VariantType.Intvector3;
            if (type == typeof(long))
                return VariantType.Int64;

            return VariantType.CustomStack;
        }

        public object Object
        {
            get
            {
                if (NativeInstance == IntPtr.Zero)
                    throw new ObjectDisposedException("Variant");

                switch (Type)
                {
                    case VariantType.None:
                        return null;
                    case VariantType.Int:
                        return Int;
                    case VariantType.Bool:
                        return Bool;
                    case VariantType.Float:
                        return Float;
                    case VariantType.Vector2:
                        return Vector2;
                    case VariantType.Vector3:
                        return Vector3;
                    case VariantType.Vector4:
                        return Vector4;
                    case VariantType.Quaternion:
                        return Quaternion;
                    case VariantType.Color:
                        return Color;
                    case VariantType.String:
                        return String;
                    case VariantType.Buffer:
                        return Buffer;
                    case VariantType.Voidptr:
                        return VoidPtr;
                    case VariantType.Resourceref:
                        return ResourceRef;
                    case VariantType.Resourcereflist:
                        return ResourceRefList;
                    //case VariantType.Variantvector:
                    //    break;
                    //case VariantType.Variantmap:
                    //    break;
                    case VariantType.Intrect:
                        return IntRect;
                    case VariantType.Intvector2:
                        return IntVector2;
                    case VariantType.Ptr:
                        return Ptr;
                    case VariantType.Matrix3:
                        return Matrix3;
                    case VariantType.Matrix3x4:
                        return Matrix3x4;
                    case VariantType.Matrix4:
                        return Matrix4;
                    case VariantType.Double:
                        return Double;
                    //case VariantType.Stringvector:
                    //    break;
                    case VariantType.Rect:
                        return Rect;
                    case VariantType.Intvector3:
                        return IntVector3;
                    case VariantType.Int64:
                        return Int64;
                    //case VariantType.CustomHeap:
                    //    break;
                    case VariantType.CustomStack:
                    {
                        var handlePtr = Urho3D_Variant__GetObject(NativeInstance);
                        if (handlePtr != IntPtr.Zero)
                            return GCHandle.FromIntPtr(handlePtr).Target;
                        break;
                    }
                }
                return null;
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
        internal static extern void Urho3D_Variant__SetObject(IntPtr variant, IntPtr value);
        #endregion
    }
}
