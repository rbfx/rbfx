//
// Copyright (c) 2008-2019 the Urho3D project.
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

#pragma once

#include "../IO/ArchiveSerialization.h"
#include "../Scene/Serializable.h"

namespace Urho3D
{

bool SerializeVariantValue(Archive& archive, VariantType variantType, const char* name, Variant& value)
{
    static_assert(MAX_VAR_TYPES == 28, "Update me");
    switch (variantType)
    {
    case VAR_NONE:
        return true;
    case VAR_INT:
        return Detail::SerializeVariantValueType<int>(archive, name, value);
    case VAR_BOOL:
        return Detail::SerializeVariantValueType<bool>(archive, name, value);
    case VAR_FLOAT:
        return Detail::SerializeVariantValueType<float>(archive, name, value);
    case VAR_VECTOR2:
        return Detail::SerializeVariantValueType<Vector2>(archive, name, value);
    case VAR_VECTOR3:
        return Detail::SerializeVariantValueType<Vector3>(archive, name, value);
    case VAR_VECTOR4:
        return Detail::SerializeVariantValueType<Vector4>(archive, name, value);
    case VAR_QUATERNION:
        return Detail::SerializeVariantValueType<Quaternion>(archive, name, value);
    case VAR_COLOR:
        return Detail::SerializeVariantValueType<Color>(archive, name, value);
    case VAR_STRING:
        return Detail::SerializeVariantValueType<ea::string>(archive, name, value);
    case VAR_BUFFER:
        if (archive.IsInput() && !value.GetBufferPtr())
            value = VariantBuffer{};
        if (auto ptr = value.GetBufferPtr())
            return SerializeVectorBytes(archive, name, "elem", *ptr);
        return false;
    case VAR_RESOURCEREF:
        return Detail::SerializeVariantValueType<ResourceRef>(archive, name, value);
    case VAR_RESOURCEREFLIST:
        return Detail::SerializeVariantValueType<ResourceRefList>(archive, name, value);
    case VAR_VARIANTVECTOR:
        if (archive.IsInput() && !value.GetVariantVectorPtr())
            value = VariantVector{};
        if (auto ptr = value.GetVariantVectorPtr())
            return SerializeVector(archive, name, nullptr, *ptr);
        return false;
    case VAR_VARIANTMAP:
        if (archive.IsInput() && !value.GetVariantMapPtr())
            value = VariantMap{};
        if (auto ptr = value.GetVariantMapPtr())
            return SerializeStringHashMap(archive, name, nullptr, *ptr);
        return false;
    case VAR_INTRECT:
        return Detail::SerializeVariantValueType<IntRect>(archive, name, value);
    case VAR_INTVECTOR2:
        return Detail::SerializeVariantValueType<IntVector2>(archive, name, value);
    case VAR_MATRIX3:
        return Detail::SerializeVariantValueType<Matrix3>(archive, name, value);
    case VAR_MATRIX3X4:
        return Detail::SerializeVariantValueType<Matrix3x4>(archive, name, value);
    case VAR_MATRIX4:
        return Detail::SerializeVariantValueType<Matrix4>(archive, name, value);
    case VAR_DOUBLE:
        return Detail::SerializeVariantValueType<double>(archive, name, value);
    case VAR_STRINGVECTOR:
        if (archive.IsInput() && !value.GetStringVectorPtr())
            value = StringVector{};
        if (auto ptr = value.GetStringVectorPtr())
            return SerializeVector(archive, name, nullptr, *ptr);
        return false;
    case VAR_RECT:
        return Detail::SerializeVariantValueType<Rect>(archive, name, value);
    case VAR_INTVECTOR3:
        return Detail::SerializeVariantValueType<IntVector3>(archive, name, value);
    case VAR_INT64:
        return Detail::SerializeVariantValueType<long long>(archive, name, value);
    case VAR_VOIDPTR:
    case VAR_PTR:
        archive.SetError(Format("Unsupported Variant type of element '{0}'", name));
        return false;
    case VAR_CUSTOM:
        if (archive.IsInput())
            value.SetCustom(SharedPtr<Serializable>{});
        if (auto ptr = value.GetCustomPtr<SharedPtr<Serializable>>())
            return SerializeValue(archive, name, *ptr);

        archive.SetError(Format("Unsupported custom Variant type of element '{0}'", name));
        return false;
    case MAX_VAR_TYPES:
    default:
        assert(0);
        return false;
    }
}

URHO3D_API bool SerializeValue(Archive& archive, const char* name, SharedPtr<Serializable>& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        // Serialize type
        StringHash type = value ? value->GetType() : StringHash{};
        const ea::string_view typeName = value ? ea::string_view{ value->GetTypeName() } : "";
        if (!SerializeStringHash(archive, "type", type, typeName))
            return false;

        // Serialize empty object
        if (type == StringHash{})
        {
            value = nullptr;
            return true;
        }

        // Create instance if loading
        if (archive.IsInput())
        {
            Context* context = archive.GetContext();
            if (!context)
            {
                archive.SetError(Format("Context is required to serialize Serializable '{0}'", name));
                return false;
            }

            value.StaticCast(context->CreateObject(type));

            if (!value)
            {
                archive.SetError(Format("Failed to create instance of type '{0}'", type.Value()));
                return false;
            }
        }

        // Serialize object
        if (ArchiveBlock valueBlock = archive.OpenUnorderedBlock("value"))
            return value->Serialize(archive);
    }
    return false;
}

}
