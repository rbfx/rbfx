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

#include "../Precompiled.h"

#include "../IO/ArchiveSerialization.h"

#include "../Core/VariantCurve.h"

namespace Urho3D
{

namespace Detail
{

ea::string NumberArrayToString(float* values, unsigned size)
{
    ea::string result;
    for (unsigned i = 0; i < size; ++i)
    {
        if (i > 0)
            result += " ";
        result += ea::string(ea::string::CtorSprintf(), "%g", values[i]);
    }
    return result;
}

ea::string NumberArrayToString(int* values, unsigned size)
{
    ea::string result;
    for (unsigned i = 0; i < size; ++i)
    {
        if (i > 0)
            result += " ";
        result += ea::to_string(values[i]);
    }
    return result;
}

unsigned StringToNumberArray(const ea::string& string, float* values, unsigned maxSize)
{
    const ea::vector<ea::string> elements = string.split(' ');
    const unsigned size = elements.size() < maxSize ? elements.size() : maxSize;
    for (unsigned i = 0; i < size; ++i)
        values[i] = ToFloat(elements[i]);

    return elements.size();
}

unsigned StringToNumberArray(const ea::string& string, int* values, unsigned maxSize)
{
    const ea::vector<ea::string> elements = string.split(' ');
    const unsigned size = elements.size() < maxSize ? elements.size() : maxSize;
    for (unsigned i = 0; i < size; ++i)
        values[i] = ToInt(elements[i]);

    return elements.size();
}

template <class T>
inline void SerializeVariantAsType(Archive& archive, const char* name, Variant& variant)
{
    if (archive.IsInput())
    {
        T value{};
        SerializeValue(archive, name, value);
        variant = value;
    }
    else
    {
        const T& value = variant.Get<T>();
        SerializeValue(archive, name, const_cast<T&>(value));
    }
}

/// ResourceRef to/from string.
struct ResourceRefStringCaster
{
    ea::string ToArchive(Archive& archive, const char* name, const ResourceRef& value) const
    {
        Context* context = archive.GetContext();
        const ea::string typeName = context->GetTypeName(value.type_);
        return Format("{};{}", typeName, value.name_);
    }

    ResourceRef FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        const ea::vector<ea::string> chunks = value.split(';', true);
        if (chunks.size() != 2)
            throw ArchiveException("Unexpected format of ResourceRef '{}/{}'", archive.GetCurrentBlockPath(), name);

        return { StringHash{chunks[0]}, chunks[1] };
    }
};

/// ResourceRefList to/from string.
struct ResourceRefListStringCaster
{
    ea::string ToArchive(Archive& archive, const char* name, const ResourceRefList& value) const
    {
        Context* context = archive.GetContext();
        const ea::string typeName = context->GetTypeName(value.type_);
        return Format("{};{}", typeName, ea::string::joined(value.names_, ";"));
    }

    ResourceRefList FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        ea::vector<ea::string> chunks = value.split(';', true);
        if (chunks.empty())
            throw ArchiveException("Unexpected format of ResourceRefList '{}/{}'", archive.GetCurrentBlockPath(), name);

        const ea::string typeName = ea::move(chunks[0]);
        chunks.pop_front();

        // Treat ";" as empty list
        if (chunks.size() == 1 && chunks[0].empty())
            chunks.clear();

        return { StringHash{typeName}, chunks };
    }
};

}

void SerializeVariantAsType(Archive& archive, const char* name, Variant& value, VariantType variantType)
{
    static_assert(MAX_VAR_TYPES == 30, "Update me");
    switch (variantType)
    {
    case VAR_NONE:
        return;

    case VAR_INT:
        Detail::SerializeVariantAsType<int>(archive, name, value);
        return;

    case VAR_BOOL:
        Detail::SerializeVariantAsType<bool>(archive, name, value);
        return;

    case VAR_FLOAT:
        Detail::SerializeVariantAsType<float>(archive, name, value);
        return;

    case VAR_VECTOR2:
        Detail::SerializeVariantAsType<Vector2>(archive, name, value);
        return;

    case VAR_VECTOR3:
        Detail::SerializeVariantAsType<Vector3>(archive, name, value);
        return;

    case VAR_VECTOR4:
        Detail::SerializeVariantAsType<Vector4>(archive, name, value);
        return;

    case VAR_QUATERNION:
        Detail::SerializeVariantAsType<Quaternion>(archive, name, value);
        return;

    case VAR_COLOR:
        Detail::SerializeVariantAsType<Color>(archive, name, value);
        return;

    case VAR_STRING:
        Detail::SerializeVariantAsType<ea::string>(archive, name, value);
        return;

    case VAR_RESOURCEREF:
        Detail::SerializeVariantAsType<ResourceRef>(archive, name, value);
        return;

    case VAR_RESOURCEREFLIST:
        Detail::SerializeVariantAsType<ResourceRefList>(archive, name, value);
        return;

    case VAR_INTRECT:
        Detail::SerializeVariantAsType<IntRect>(archive, name, value);
        return;

    case VAR_INTVECTOR2:
        Detail::SerializeVariantAsType<IntVector2>(archive, name, value);
        return;

    case VAR_MATRIX3:
        Detail::SerializeVariantAsType<Matrix3>(archive, name, value);
        return;

    case VAR_MATRIX3X4:
        Detail::SerializeVariantAsType<Matrix3x4>(archive, name, value);
        return;

    case VAR_MATRIX4:
        Detail::SerializeVariantAsType<Matrix4>(archive, name, value);
        return;

    case VAR_DOUBLE:
        Detail::SerializeVariantAsType<double>(archive, name, value);
        return;

    case VAR_RECT:
        Detail::SerializeVariantAsType<Rect>(archive, name, value);
        return;

    case VAR_INTVECTOR3:
        Detail::SerializeVariantAsType<IntVector3>(archive, name, value);
        return;

    case VAR_INT64:
        Detail::SerializeVariantAsType<long long>(archive, name, value);
        return;

    case VAR_VARIANTCURVE:
        Detail::SerializeVariantAsType<VariantCurve>(archive, name, value);
        return;

    case VAR_BUFFER:
    {
        if (archive.IsInput() && !value.GetBufferPtr())
            value = VariantBuffer{};

        auto ptr = value.GetBufferPtr();
        URHO3D_ASSERT(ptr, "Cannot save Variant of mismatching type");
        SerializeVectorAsBytes(archive, name, *ptr);
        return;
    }

    case VAR_VARIANTVECTOR:
    {
        if (archive.IsInput() && !value.GetVariantVectorPtr())
            value = VariantVector{};

        auto ptr = value.GetVariantVectorPtr();
        URHO3D_ASSERT(ptr, "Cannot save Variant of mismatching type");
        SerializeVectorAsObjects(archive, name, *ptr);
        return;
    }

    case VAR_VARIANTMAP:
    {
        if (archive.IsInput() && !value.GetVariantMapPtr())
            value = VariantMap{};

        auto ptr = value.GetVariantMapPtr();
        URHO3D_ASSERT(ptr, "Cannot save Variant of mismatching type");
        SerializeMap(archive, name, *ptr);
        return;
    }

    case VAR_STRINGVECTOR:
    {
        if (archive.IsInput() && !value.GetStringVectorPtr())
            value = StringVector{};

        auto ptr = value.GetStringVectorPtr();
        URHO3D_ASSERT(ptr, "Cannot save Variant of mismatching type");
        SerializeVectorAsObjects(archive, name, *ptr);
        return;
    }

    case VAR_CUSTOM:
    {
        // Even if loading, value should be initialized to default value.
        // It's the only way to know type.
        CustomVariantValue* ptr = value.GetCustomVariantValuePtr();
        URHO3D_ASSERT(ptr, "Cannot serialize CustomVariant of unknown type");

        ptr->Serialize(archive, name);
        return;
    }

    case VAR_STRINGVARIANTMAP:
    {
        if (archive.IsInput() && !value.GetStringVariantMapPtr())
            value = StringVariantMap{};

        auto ptr = value.GetStringVariantMapPtr();
        URHO3D_ASSERT(ptr, "Cannot save Variant of mismatching type");
        SerializeMap(archive, name, *ptr);
        return;
    }

    case VAR_VOIDPTR:
    case VAR_PTR:
        throw ArchiveException("'{}/{}' has unsupported variant type");

    case MAX_VAR_TYPES:
    default:
        URHO3D_ASSERT(0);
    }
}

void SerializeValue(Archive& archive, const char* name, StringVector& value)
{
    SerializeVectorAsObjects(archive, name, value);
}

void SerializeValue(Archive& archive, const char* name, VariantVector& value)
{
    SerializeVectorAsObjects(archive, name, value);
}

void SerializeValue(Archive& archive, const char* name, VariantMap& value)
{
    SerializeMap(archive, name, value);
}

void SerializeValue(Archive& archive, const char* name, StringVariantMap& value)
{
    SerializeMap(archive, name, value);
}

void SerializeValue(Archive& archive, const char* name, ResourceRef& value)
{
    if (!archive.IsHumanReadable())
    {
        ArchiveBlock block = archive.OpenUnorderedBlock(name);
        SerializeValue(archive, "type", value.type_);
        SerializeValue(archive, "name", value.name_);
        return;
    }

    SerializeValueAsType<ea::string>(archive, name, value, Detail::ResourceRefStringCaster{});
}

void SerializeValue(Archive& archive, const char* name, ResourceRefList& value)
{
    if (!archive.IsHumanReadable())
    {
        ArchiveBlock block = archive.OpenUnorderedBlock(name);
        SerializeValue(archive, "type", value.type_);
        SerializeVectorAsObjects(archive, "names", value.names_);
        return;
    }

    SerializeValueAsType<ea::string>(archive, name, value, Detail::ResourceRefListStringCaster{});
}

}
