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

namespace Urho3DNet
{
    public partial class Variant
    {
        public VariantType Type
        {
            get { return GetVariantType(); }
        }

        public static implicit operator int(Variant value)
        {
            if (value.Type != VariantType.VarInt)
                return value.Convert(VariantType.VarInt).Int;
            return value.Int;
        }

        public static implicit operator Variant(int value)
        {
            return new Variant(value);
        }

        public static implicit operator long(Variant value)
        {
            if (value.Type != VariantType.VarInt64)
                return value.Convert(VariantType.VarInt64).Int;
            return value.Int;
        }

        public static implicit operator Variant(long value)
        {
            return new Variant(value);
        }

        public static implicit operator uint(Variant value)
        {
            if (value.Type != VariantType.VarInt)
                return (uint)value.Convert(VariantType.VarInt).Int;
            return (uint)value.Int;
        }

        public static implicit operator Variant(uint value)
        {
            return new Variant(value);
        }

        public static implicit operator ulong(Variant value)
        {
            if (value.Type != VariantType.VarInt64)
                return (ulong)value.Convert(VariantType.VarInt64).Int;
            return (ulong)value.Int;
        }

        public static implicit operator Variant(ulong value)
        {
            return new Variant(value);
        }

        public static implicit operator StringHash(Variant value)
        {
            return new StringHash((uint)value);
        }

        public static implicit operator Variant(StringHash value)
        {
            return new Variant(value);
        }

        public static implicit operator bool(Variant value)
        {
            if (value.Type != VariantType.VarBool)
                return value.Convert(VariantType.VarBool).Bool;
            return value.Bool;
        }

        public static implicit operator Variant(bool value)
        {
            return new Variant(value);
        }

        public static implicit operator float(Variant value)
        {
            if (value.Type != VariantType.VarFloat)
                return value.Convert(VariantType.VarFloat).Float;
            return value.Float;
        }

        public static implicit operator Variant(float value)
        {
            return new Variant(value);
        }

        public static implicit operator double(Variant value)
        {
            if (value.Type != VariantType.VarDouble)
                return value.Convert(VariantType.VarDouble).Double;
            return value.Double;
        }

        public static implicit operator Variant(double value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Vector2(Variant value)
        {
            if (value.Type != VariantType.VarVector2)
                return value.Convert(VariantType.VarVector2).Vector2;
            return value.Vector2;
        }

        public static implicit operator Variant(global::Urho3DNet.Vector2 value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Vector3(Variant value)
        {
            if (value.Type != VariantType.VarVector3)
                return value.Convert(VariantType.VarVector3).Vector3;
            return value.Vector3;
        }

        public static implicit operator Variant(global::Urho3DNet.Vector3 value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Vector4(Variant value)
        {
            return (value.Type != VariantType.VarVector4 ? value.Convert(VariantType.VarVector4) : value).Vector4;
        }

        public static implicit operator Variant(global::Urho3DNet.Vector4 value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Quaternion(Variant value)
        {
            if (value.Type != VariantType.VarQuaternion)
                return value.Convert(VariantType.VarQuaternion).Quaternion;
            return value.Quaternion;
        }

        public static implicit operator Variant(global::Urho3DNet.Quaternion value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Color(Variant value)
        {
            if (value.Type != VariantType.VarColor)
                return value.Convert(VariantType.VarColor).Color;
            return value.Color;
        }

        public static implicit operator Variant(global::Urho3DNet.Color value)
        {
            return new Variant(value);
        }

        public static implicit operator string(Variant value)
        {
            if (value.Type != VariantType.VarString)
                return value.Convert(VariantType.VarString).String;
            return value.String;
        }

        public static implicit operator Variant(string value)
        {
            return new Variant(value ?? string.Empty);
        }

        public static implicit operator ByteVector(Variant value)
        {
            if (value.Type != VariantType.VarBuffer)
                return value.Convert(VariantType.VarBuffer).Buffer;
            return value.Buffer;
        }

        public static implicit operator Variant(byte[] value)
        {
            return new Variant(value ?? Array.Empty<byte>());
        }

        public static implicit operator global::System.IntPtr(Variant value)
        {
            if (value.Type != VariantType.VarVoidPtr)
                return value.Convert(VariantType.VarVoidPtr).VoidPtr;
            return value.VoidPtr;
        }

        public static implicit operator Variant(global::System.IntPtr value)
        {
            return new Variant(value);
        }

        public static implicit operator ResourceRef(Variant value)
        {
            if (value.Type != VariantType.VarResourceRef)
                return value.Convert(VariantType.VarResourceRef).ResourceRef;
            return value.ResourceRef;
        }

        public static implicit operator Variant(ResourceRef value)
        {
            return new Variant(value);
        }

        public static implicit operator ResourceRefList(Variant value)
        {
            if (value.Type != VariantType.VarResourceRefList)
                return value.Convert(VariantType.VarResourceRefList).ResourceRefList;
            return value.ResourceRefList;
        }

        public static implicit operator Variant(ResourceRefList value)
        {
            return new Variant(value);
        }

        public static implicit operator VariantList(Variant value)
        {
            if (value.Type != VariantType.VarVariantList)
                return value.Convert(VariantType.VarVariantList).VariantVector;
            return value.VariantVector;
        }

        public static implicit operator Variant(VariantList value)
        {
            return new Variant(value);
        }

        public static implicit operator VariantMap(Variant value)
        {
            if (value.Type != VariantType.VarVariantMap)
                return value.Convert(VariantType.VarVariantMap).VariantMap;
            return value.VariantMap;
        }

        public static implicit operator Variant(VariantMap value)
        {
            return new Variant(value);
        }

        public static implicit operator StringList(Variant value)
        {
            if (value.Type != VariantType.VarStringList)
                return value.Convert(VariantType.VarStringList).StringVector;
            return value.StringVector;
        }

        public static implicit operator Variant(StringList value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Rect(Variant value)
        {
            return (value.Type != VariantType.VarRect ? value.Convert(VariantType.VarRect) : value).Rect;
        }

        public static implicit operator Variant(global::Urho3DNet.Rect value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.IntRect(Variant value)
        {
            return (value.Type != VariantType.VarIntRect ? value.Convert(VariantType.VarIntRect) : value).IntRect;
        }

        public static implicit operator Variant(global::Urho3DNet.IntRect value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.IntVector2(Variant value)
        {
            return (value.Type != VariantType.VarIntVector2 ? value.Convert(VariantType.VarIntVector2) : value).IntVector2;
        }

        public static implicit operator Variant(global::Urho3DNet.IntVector2 value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.IntVector3(Variant value)
        {
            return (value.Type != VariantType.VarIntVector3 ? value.Convert(VariantType.VarIntVector3) : value).IntVector3;
        }

        public static implicit operator Variant(global::Urho3DNet.IntVector3 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(RefCounted value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Matrix3(Variant value)
        {
            return (value.Type != VariantType.VarMatrix3? value.Convert(VariantType.VarMatrix3) : value).Matrix3;
        }

        public static implicit operator Variant(global::Urho3DNet.Matrix3 value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Matrix3x4(Variant value)
        {
            return (value.Type != VariantType.VarMatrix3x4 ? value.Convert(VariantType.VarMatrix3x4) : value).Matrix3x4;
        }

        public static implicit operator Variant(global::Urho3DNet.Matrix3x4 value)
        {
            return new Variant(value);
        }

        public static implicit operator global::Urho3DNet.Matrix4(Variant value)
        {
            return (value.Type != VariantType.VarMatrix4 ? value.Convert(VariantType.VarMatrix4) : value).Matrix4;
        }

        public static implicit operator Variant(global::Urho3DNet.Matrix4 value)
        {
            return new Variant(value);
        }

        public static VariantType GetVariantType(System.Type type)
        {
            if (type == typeof(sbyte) || type == typeof(short) || type == typeof(int))
                return VariantType.VarInt;
            if (type == typeof(bool))
                return VariantType.VarBool;
            if (type == typeof(float))
                return VariantType.VarFloat;
            if (type == typeof(Vector2))
                return VariantType.VarVector2;
            if (type == typeof(Vector3))
                return VariantType.VarVector3;
            if (type == typeof(Vector4))
                return VariantType.VarVector4;
            if (type == typeof(Quaternion))
                return VariantType.VarQuaternion;
            if (type == typeof(Color))
                return VariantType.VarColor;
            if (type == typeof(string))
                return VariantType.VarString;
            if (type == typeof(ByteVector))
                return VariantType.VarBuffer;
            if (type == typeof(System.IntPtr))
                return VariantType.VarVoidPtr;
            if (type == typeof(ResourceRef))
                return VariantType.VarResourceRef;
            if (type == typeof(ResourceRefList))
                return VariantType.VarResourceRefList;
            if (type == typeof(VariantList))
                return VariantType.VarVariantList;
            if (type == typeof(VariantMap))
                return VariantType.VarVariantMap;
            if (type == typeof(IntRect))
                return VariantType.VarIntRect;
            if (type == typeof(IntVector2))
                return VariantType.VarIntVector2;
            if (type == typeof(RefCounted))
                return VariantType.VarPtr;
            if (type == typeof(Matrix3))
                return VariantType.VarMatrix3;
            if (type == typeof(Matrix3x4))
                return VariantType.VarMatrix3x4;
            if (type == typeof(Matrix4))
                return VariantType.VarMatrix4;
            if (type == typeof(double))
                return VariantType.VarDouble;
            if (type == typeof(StringList))
                return VariantType.VarStringList;
            if (type == typeof(Rect))
                return VariantType.VarRect;
            if (type == typeof(IntVector3))
                return VariantType.VarIntVector3;
            if (type == typeof(long))
                return VariantType.VarInt64;
            return VariantType.VarNone;
        }
    }
}
