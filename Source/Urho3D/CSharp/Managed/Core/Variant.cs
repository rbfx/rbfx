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

namespace Urho3DNet
{
    public partial class Variant
    {
        public static implicit operator Variant(int value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(long value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(uint value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(ulong value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(StringHash value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(bool value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(float value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(double value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Vector2 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Vector3 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Vector4 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Quaternion value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Color value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(string value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(byte[] value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::System.IntPtr value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(ResourceRef value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(ResourceRefList value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(VariantVector value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(VariantMap value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(StringVector value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Rect value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.IntRect value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.IntVector2 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.IntVector3 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(RefCounted value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Matrix3 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Matrix3x4 value)
        {
            return new Variant(value);
        }

        public static implicit operator Variant(global::Urho3DNet.Matrix4 value)
        {
            return new Variant(value);
        }
    }
}
