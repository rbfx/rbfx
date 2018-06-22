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
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using Urho3D.CSharp;

namespace Urho3D
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public class AttributeAttribute : Attribute
    {
        public string Name;
        public object DefaultValue;
        public AttributeMode Mode = AttributeMode.Default;
    }

    public partial class Serializable
    {
        internal delegate IntPtr AttributeGetterDelegate(IntPtr serializable);
        internal delegate void AttributeSetterDelegate(IntPtr serializable, IntPtr variant);

        // Keep references to all getters and setters to prevent GC freeing them.
        private static List<GCHandle> _attributeAccessorReferences = new List<GCHandle>();

        internal static void RegisterSerializableAttributes(Context context)
        {
            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach(var type in assembly.GetTypes())
                {
                    if (!type.IsSubclassOf(typeof(Serializable)))
                        continue;

                    foreach (var fieldInfo in type.GetFields())
                    {
                        var attributes = (AttributeAttribute[]) fieldInfo.GetCustomAttributes(typeof(AttributeAttribute), true);
                        foreach (var attribute in attributes)
                        {
                            AttributeGetterDelegate attributeGetter = serializable =>
                            {
                                var value = fieldInfo.GetValue(GetManagedInstance(serializable));
                                return Variant.GetNativeInstance(Variant.FromObject(value));
                            };

                            AttributeSetterDelegate attributeSetter = (serializable, variant) =>
                            {
                                fieldInfo.SetValue(GetManagedInstance(serializable),
                                    Variant.GetManagedInstance(variant).Object);
                            };

                            _attributeAccessorReferences.Add(GCHandle.Alloc(attributeGetter));
                            _attributeAccessorReferences.Add(GCHandle.Alloc(attributeSetter));

                            var typeHash = new StringHash(type);
                            Urho3D_Serializable_RegisterAttribute(Context.GetNativeInstance(context), typeHash.Hash,
                                Variant.GetVariantType(fieldInfo.FieldType), attribute.Name ?? fieldInfo.Name,
                                Variant.GetNativeInstance(Variant.FromObject(attribute.DefaultValue)), attribute.Mode,
                                fieldInfo.FieldType.IsEnum ? fieldInfo.FieldType.GetEnumNames() : null,
                                Marshal.GetFunctionPointerForDelegate(attributeGetter),
                                Marshal.GetFunctionPointerForDelegate(attributeSetter));
                        }
                    }

                    foreach (var propertyInfo in type.GetProperties())
                    {
                        var attributes = (AttributeAttribute[]) propertyInfo.GetCustomAttributes(typeof(AttributeAttribute), true);
                        foreach (var attribute in attributes)
                        {
                            AttributeGetterDelegate attributeGetter = serializable =>
                            {
                                var value = propertyInfo.GetValue(GetManagedInstance(serializable));
                                return Variant.GetNativeInstance(Variant.FromObject(value));
                            };

                            AttributeSetterDelegate attributeSetter = (serializable, variant) =>
                            {
                                propertyInfo.SetValue(GetManagedInstance(serializable),
                                    Variant.GetManagedInstance(variant).Object);
                            };

                            _attributeAccessorReferences.Add(GCHandle.Alloc(attributeGetter));
                            _attributeAccessorReferences.Add(GCHandle.Alloc(attributeSetter));

                            var typeHash = new StringHash(type);
                            Urho3D_Serializable_RegisterAttribute(Context.GetNativeInstance(context), typeHash.Hash,
                                Variant.GetVariantType(propertyInfo.PropertyType), attribute.Name ?? propertyInfo.Name,
                                Variant.GetNativeInstance(Variant.FromObject(attribute.DefaultValue)), attribute.Mode,
                                propertyInfo.PropertyType.IsEnum ? propertyInfo.PropertyType.GetEnumNames() : null,
                                Marshal.GetFunctionPointerForDelegate(attributeGetter),
                                Marshal.GetFunctionPointerForDelegate(attributeSetter));
                        }
                    }
                }
            }
        }

        #region Interop

        [SuppressUnmanagedCodeSecurity]
        [DllImport(Urho3D.CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_Serializable_RegisterAttribute(IntPtr context, uint typeHash, VariantType type,
            [param: MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8))]string name,
            IntPtr defaultValue, AttributeMode mode,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(StringUtf8ArrayMarshaller))]string[] enumNames,
            IntPtr getter, IntPtr setter);

        #endregion
    }
}
