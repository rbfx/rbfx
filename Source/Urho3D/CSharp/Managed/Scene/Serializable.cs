//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

namespace Urho3DNet
{
    /// <summary>
    /// Marks field as serializable. May be used to customize attribute name or mode as well as forcing serialization of
    /// private fields.
    /// </summary>
    public class SerializeFieldAttribute : System.Attribute
    {
        /// <summary>
        /// Name which will be used for exposing field to the engine. If no name is provided a name of field will be used.
        /// </summary>
        public string Name = null;
        /// <summary>
        /// Attribute mode defines various properties like serializability, editor visibility, network synchronization.
        /// </summary>
        public AttributeMode Mode = AttributeMode.AmDefault;
    }

    internal class VariantFieldAccessor : AttributeAccessor
    {
        private FieldInfo _field;
        public VariantType VariantType;

        public VariantFieldAccessor(FieldInfo field, VariantType type)
        {
            _field = field;
            VariantType = type;
        }

        public override void Get(Serializable ptr, Variant dest)
        {
            switch (VariantType)
            {
                case VariantType.VarNone:
                    dest.Clear();
                    break;
                case VariantType.VarInt:
                    dest.Set((int) _field.GetValue(ptr));
                    break;
                case VariantType.VarBool:
                    dest.Set((bool) _field.GetValue(ptr));
                    break;
                case VariantType.VarFloat:
                    dest.Set((float) _field.GetValue(ptr));
                    break;
                case VariantType.VarVector2:
                    dest.Set((Vector2) _field.GetValue(ptr));
                    break;
                case VariantType.VarVector3:
                    dest.Set((Vector3) _field.GetValue(ptr));
                    break;
                case VariantType.VarVector4:
                    dest.Set((Vector4) _field.GetValue(ptr));
                    break;
                case VariantType.VarQuaternion:
                    dest.Set((Quaternion) _field.GetValue(ptr));
                    break;
                case VariantType.VarColor:
                    dest.Set((Color) _field.GetValue(ptr));
                    break;
                case VariantType.VarString:
                    dest.Set((string) _field.GetValue(ptr));
                    break;
                case VariantType.VarBuffer:
                    dest.Set((UCharArray) _field.GetValue(ptr));
                    break;
                case VariantType.VarVoidptr:
                    dest.Set((IntPtr) _field.GetValue(ptr));
                    break;
                case VariantType.VarResourceref:
                    dest.Set((ResourceRef) _field.GetValue(ptr));
                    break;
                case VariantType.VarResourcereflist:
                    dest.Set((ResourceRefList) _field.GetValue(ptr));
                    break;
                case VariantType.VarVariantvector:
                    dest.Set((VariantList) _field.GetValue(ptr));
                    break;
                case VariantType.VarVariantmap:
                    dest.Set((VariantMap) _field.GetValue(ptr));
                    break;
                case VariantType.VarIntrect:
                    dest.Set((IntRect) _field.GetValue(ptr));
                    break;
                case VariantType.VarIntvector2:
                    dest.Set((IntVector2) _field.GetValue(ptr));
                    break;
                case VariantType.VarPtr:
                    dest.Set((RefCounted) _field.GetValue(ptr));
                    break;
                case VariantType.VarMatrix3:
                    dest.Set((Matrix3) _field.GetValue(ptr));
                    break;
                case VariantType.VarMatrix3x4:
                    dest.Set((Matrix3x4) _field.GetValue(ptr));
                    break;
                case VariantType.VarMatrix4:
                    dest.Set((Matrix4) _field.GetValue(ptr));
                    break;
                case VariantType.VarDouble:
                    dest.Set((double) _field.GetValue(ptr));
                    break;
                case VariantType.VarStringvector:
                    dest.Set((StringList) _field.GetValue(ptr));
                    break;
                case VariantType.VarRect:
                    dest.Set((Rect) _field.GetValue(ptr));
                    break;
                case VariantType.VarIntvector3:
                    dest.Set((IntVector3) _field.GetValue(ptr));
                    break;
                case VariantType.VarInt64:
                    dest.Set((long) _field.GetValue(ptr));
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public override void Set(Serializable ptr, Variant src)
        {
            if (src == null || src.GetVariantType() == VariantType.VarNone)
            {
                _field.SetValue(ptr, null);
                return;
            }

            switch (VariantType)
            {
                case VariantType.VarNone:
                    // Undetermined field type.
                    return;
                case VariantType.VarInt:
                    _field.SetValue(ptr, src.GetInt());
                    break;
                case VariantType.VarBool:
                    _field.SetValue(ptr, src.GetBool());
                    break;
                case VariantType.VarFloat:
                    _field.SetValue(ptr, src.GetFloat());
                    break;
                case VariantType.VarVector2:
                    _field.SetValue(ptr, src.GetVector2());
                    break;
                case VariantType.VarVector3:
                    _field.SetValue(ptr, src.GetVector3());
                    break;
                case VariantType.VarVector4:
                    _field.SetValue(ptr, src.GetVector4());
                    break;
                case VariantType.VarQuaternion:
                    _field.SetValue(ptr, src.GetQuaternion());
                    break;
                case VariantType.VarColor:
                    _field.SetValue(ptr, src.GetColor());
                    break;
                case VariantType.VarString:
                    _field.SetValue(ptr, src.GetString());
                    break;
                case VariantType.VarBuffer:
                    _field.SetValue(ptr, src.GetBuffer());
                    break;
                case VariantType.VarVoidptr:
                    _field.SetValue(ptr, src.GetVoidPtr());
                    break;
                case VariantType.VarResourceref:
                    _field.SetValue(ptr, src.GetResourceRef());
                    break;
                case VariantType.VarResourcereflist:
                    _field.SetValue(ptr, src.GetResourceRefList());
                    break;
                case VariantType.VarVariantvector:
                    _field.SetValue(ptr, src.GetVariantVector());
                    break;
                case VariantType.VarVariantmap:
                    _field.SetValue(ptr, src.GetVariantMap());
                    break;
                case VariantType.VarIntrect:
                    _field.SetValue(ptr, src.GetIntRect());
                    break;
                case VariantType.VarIntvector2:
                    _field.SetValue(ptr, src.GetIntVector2());
                    break;
                case VariantType.VarPtr:
                    _field.SetValue(ptr, src.GetPtr());
                    break;
                case VariantType.VarMatrix3:
                    _field.SetValue(ptr, src.GetMatrix3());
                    break;
                case VariantType.VarMatrix3x4:
                    _field.SetValue(ptr, src.GetMatrix3x4());
                    break;
                case VariantType.VarMatrix4:
                    _field.SetValue(ptr, src.GetMatrix4());
                    break;
                case VariantType.VarDouble:
                    _field.SetValue(ptr, src.GetDouble());
                    break;
                case VariantType.VarStringvector:
                    _field.SetValue(ptr, src.GetStringVector());
                    break;
                case VariantType.VarRect:
                    _field.SetValue(ptr, src.GetRect());
                    break;
                case VariantType.VarIntvector3:
                    _field.SetValue(ptr, src.GetIntVector3());
                    break;
                case VariantType.VarInt64:
                    _field.SetValue(ptr, src.GetInt64());
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }
    }

    public partial class Serializable
    {
        private static StringList _emptyStringList = new StringList();

        protected void OnSetupInstance()
        {
            var type = GetType();
            var serializableType = typeof(Serializable);
            // Only do this for user types.
            if (type.Assembly == serializableType.Assembly || !type.IsSubclassOf(serializableType))
                return;

            var context = GetContext();
            var allAttributes = context.GetAllAttributes();
            // And only once per type.
            if (allAttributes.ContainsKey(GetTypeHash()))
                return;

            // Register attributes of this class
            foreach (var field in type.GetFields(BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic))
            {
                if (field.DeclaringType?.Assembly == serializableType.Assembly)
                    continue;

                if (field.IsNotSerialized)
                    continue;

                var attribute = Attribute.GetCustomAttribute(field, typeof(SerializeFieldAttribute)) as SerializeFieldAttribute;
                if (field.IsPrivate && attribute == null)
                    continue;

                var fieldType = field.FieldType;
                if (field.FieldType.IsEnum)
                    fieldType = Enum.GetUnderlyingType(fieldType);
                var variantType = Variant.GetVariantType(fieldType);
                if (variantType == VariantType.VarNone)
                    // Incompatible type.
                    continue;

                // TODO: This needs more work. Serializable should automatically serialize instances of Serializable.
                if (variantType == VariantType.VarPtr)
                    continue;

                // No way to serialize that in any meaningful way.
                if (variantType == VariantType.VarVoidptr)
                    continue;

                var enumNames = _emptyStringList;
                if (field.FieldType.IsEnum)
                {
                    enumNames = new StringList();
                    foreach (var name in field.FieldType.GetEnumNames())
                        enumNames.Add(name);
                }

                var accessor = new VariantFieldAccessor(field, variantType);
                var defaultValue = new Variant();
                accessor.Get(this, defaultValue);

                var attributeName = attribute?.Name ?? field.Name;
                var info = new AttributeInfo(accessor.VariantType, attributeName, accessor, enumNames, defaultValue,
                    attribute?.Mode ?? AttributeMode.AmDefault);
                context.RegisterAttribute(GetTypeHash(), info);
            }
        }
    }
}
