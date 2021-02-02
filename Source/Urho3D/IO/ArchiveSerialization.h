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

#pragma once

#include "../Core/Object.h"
#include "../Core/StringUtils.h"
#include "../Core/Variant.h"
#include "../IO/Archive.h"
#include "../Math/Color.h"
#include "../Math/Matrix3.h"
#include "../Math/Matrix3x4.h"
#include "../Math/Matrix4.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"

#include <EASTL/string.h>

#include <type_traits>

namespace Urho3D
{

namespace Detail
{

/// Format float array to string.
inline ea::string FormatArray(float* values, unsigned size)
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

/// Format int array to string.
inline ea::string FormatArray(int* values, unsigned size)
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

/// Un-format float array from string.
inline unsigned UnFormatArray(const ea::string& string, float* values, unsigned maxSize)
{
    const ea::vector<ea::string> elements = string.split(' ');
    const unsigned size = elements.size() < maxSize ? elements.size() : maxSize;
    for (unsigned i = 0; i < size; ++i)
        values[i] = ToFloat(elements[i]);

    return elements.size();
}

/// Un-format int array from string.
inline unsigned UnFormatArray(const ea::string& string, int* values, unsigned maxSize)
{
    const ea::vector<ea::string> elements = string.split(' ');
    const unsigned size = elements.size() < maxSize ? elements.size() : maxSize;
    for (unsigned i = 0; i < size; ++i)
        values[i] = ToInt(elements[i]);

    return elements.size();
}

/// Serialize array of fixed size.
template <class T>
inline bool SerializeArray(Archive& archive, const char* name, T* values, unsigned size)
{
    if (!archive.IsHumanReadable())
        return archive.SerializeBytes(name, values, size * sizeof(T));
    else
    {
        if (archive.IsInput())
        {
            ea::string string;
            if (!archive.Serialize(name, string))
                return false;

            const unsigned realSize = Detail::UnFormatArray(string, values, size);
            return realSize == size;
        }
        else
        {
            ea::string string = Detail::FormatArray(values, size);
            return archive.Serialize(name, string);
        }
    }
}

/// Serialize type as fixed array.
template <unsigned N, class T>
inline bool SerializeArrayType(Archive& archive, const char* name, T& value)
{
    using ElementType = std::decay_t<decltype(*value.Data())>;

    if (archive.IsInput())
    {
        ElementType elements[N];
        if (!SerializeArray(archive, name, elements, N))
            return false;

        value = T{ elements };
        return true;
    }
    else
    {
        return SerializeArray(archive, name, const_cast<ElementType*>(value.Data()), N);
    }
}

/// Serialize value of the Variant (of specific type).
template <class T>
inline bool SerializeVariantValueType(Archive& archive, const char* name, Variant& variant)
{
    if (archive.IsInput())
    {
        T value{};
        if (!SerializeValue(archive, name, value))
            return false;
        variant = value;
        return true;
    }
    else
    {
        const T& value = variant.Get<T>();
        return SerializeValue(archive, name, const_cast<T&>(value));
    }
}

/// Format ResourceRefList to string.
inline ea::string FormatResourceRefList(ea::string_view typeString, const ea::string* names, unsigned size)
{
    ea::string result{ typeString };
    for (unsigned i = 0; i < size; ++i)
    {
        result += ";";
        result += names[i];
    }
    return result;
}

}

/// Serialize bool.
inline bool SerializeValue(Archive& archive, const char* name, bool& value) { return archive.Serialize(name, value); }

/// Serialize signed char.
inline bool SerializeValue(Archive& archive, const char* name, signed char& value) { return archive.Serialize(name, value); }

/// Serialize unsigned char.
inline bool SerializeValue(Archive& archive, const char* name, unsigned char& value) { return archive.Serialize(name, value); }

/// Serialize signed short.
inline bool SerializeValue(Archive& archive, const char* name, short& value) { return archive.Serialize(name, value); }

/// Serialize unsigned short.
inline bool SerializeValue(Archive& archive, const char* name, unsigned short& value) { return archive.Serialize(name, value); }

/// Serialize signed int.
inline bool SerializeValue(Archive& archive, const char* name, int& value) { return archive.Serialize(name, value); }

/// Serialize unsigned int.
inline bool SerializeValue(Archive& archive, const char* name, unsigned int& value) { return archive.Serialize(name, value); }

/// Serialize signed long.
inline bool SerializeValue(Archive& archive, const char* name, long long& value) { return archive.Serialize(name, value); }

/// Serialize unsigned long.
inline bool SerializeValue(Archive& archive, const char* name, unsigned long long& value) { return archive.Serialize(name, value); }

/// Serialize float.
inline bool SerializeValue(Archive& archive, const char* name, float& value) { return archive.Serialize(name, value); }

/// Serialize double.
inline bool SerializeValue(Archive& archive, const char* name, double& value) { return archive.Serialize(name, value); }

/// Serialize string.
inline bool SerializeValue(Archive& archive, const char* name, ea::string& value) { return archive.Serialize(name, value); }

/// Serialize Vector2.
inline bool SerializeValue(Archive& archive, const char* name, Vector2& value)
{
    return Detail::SerializeArrayType<2>(archive, name, value);
}

/// Serialize Vector3.
inline bool SerializeValue(Archive& archive, const char* name, Vector3& value)
{
    return Detail::SerializeArrayType<3>(archive, name, value);
}

/// Serialize Vector4.
inline bool SerializeValue(Archive& archive, const char* name, Vector4& value)
{
    return Detail::SerializeArrayType<4>(archive, name, value);
}

/// Serialize Matrix3.
inline bool SerializeValue(Archive& archive, const char* name, Matrix3& value)
{
    return Detail::SerializeArrayType<9>(archive, name, value);
}

/// Serialize Matrix3x4.
inline bool SerializeValue(Archive& archive, const char* name, Matrix3x4& value)
{
    return Detail::SerializeArrayType<12>(archive, name, value);
}

/// Serialize Matrix4.
inline bool SerializeValue(Archive& archive, const char* name, Matrix4& value)
{
    return Detail::SerializeArrayType<16>(archive, name, value);
}

/// Serialize Rect.
inline bool SerializeValue(Archive& archive, const char* name, Rect& value)
{
    return Detail::SerializeArrayType<4>(archive, name, value);
}

/// Serialize Quaternion.
inline bool SerializeValue(Archive& archive, const char* name, Quaternion& value)
{
    return Detail::SerializeArrayType<4>(archive, name, value);
}

/// Serialize Color.
inline bool SerializeValue(Archive& archive, const char* name, Color& value)
{
    return Detail::SerializeArrayType<4>(archive, name, value);
}

/// Serialize IntVector2.
inline bool SerializeValue(Archive& archive, const char* name, IntVector2& value)
{
    return Detail::SerializeArrayType<2>(archive, name, value);
}

/// Serialize IntVector3.
inline bool SerializeValue(Archive& archive, const char* name, IntVector3& value)
{
    return Detail::SerializeArrayType<3>(archive, name, value);
}

/// Serialize IntRect.
inline bool SerializeValue(Archive& archive, const char* name, IntRect& value)
{
    return Detail::SerializeArrayType<4>(archive, name, value);
}

/// Serialize StringHash (as is).
inline bool SerializeValue(Archive& archive, const char* name, StringHash& value)
{
    unsigned hashValue = value.Value();
    if (!SerializeValue(archive, name, hashValue))
        return false;
    value = StringHash{ hashValue };
    return true;
}

/// Serialize enum as integer (if binary archive) or as string (if text archive).
template <class EnumType, class UnderlyingInteger = std::underlying_type_t<EnumType>>
inline bool SerializeEnum(Archive& archive, const char* name, const char* const* enumConstants, EnumType& value)
{
    assert(enumConstants);
    const bool loading = archive.IsInput();
    if (!archive.IsHumanReadable())
    {
        if (loading)
        {
            UnderlyingInteger intValue{};
            if (!SerializeValue(archive, name, intValue))
                return false;
            value = static_cast<EnumType>(intValue);
            return true;
        }
        else
        {
            UnderlyingInteger intValue = static_cast<UnderlyingInteger>(value);
            if (!SerializeValue(archive, name, intValue))
                return false;
            return true;
        }
    }
    else
    {
        if (loading)
        {
            ea::string stringValue;
            if (!SerializeValue(archive, name, stringValue))
                return false;
            value = static_cast<EnumType>(GetStringListIndex(stringValue.c_str(), enumConstants, 0));
            return true;
        }
        else
        {
            // This is so unsafe
            ea::string stringValue = enumConstants[static_cast<UnderlyingInteger>(value)];
            return SerializeValue(archive, name, stringValue);
        }
    }
}

/// Serialize string hash as integer or as string.
inline bool SerializeStringHash(Archive& archive, const char* name, StringHash& value, const ea::string_view string)
{
    if (!archive.IsHumanReadable())
    {
        unsigned hashValue = value.Value();
        if (!SerializeValue(archive, name, hashValue))
            return false;
        value = StringHash{ hashValue };
    }
    else
    {
        if (archive.IsInput())
        {
            ea::string stringValue;
            if (!SerializeValue(archive, name, stringValue))
                return false;
            value = StringHash{ stringValue };
        }
        else
        {
            ea::string stringValue{ string };
            if (!SerializeValue(archive, name, stringValue))
                return false;
        }
    }
    return true;
}

/// Serialize string hash map key as integer or as string.
inline bool SerializeStringHashKey(Archive& archive, StringHash& value, const ea::string_view string)
{
    if (!archive.IsHumanReadable())
    {
        unsigned hashValue = value.Value();
        if (!archive.SerializeKey(hashValue))
            return false;
        value = StringHash{ hashValue };
    }
    else
    {
        if (archive.IsInput())
        {
            ea::string stringValue;
            if (!archive.SerializeKey(stringValue))
                return false;
            value = StringHash{ stringValue };
        }
        else
        {
            ea::string stringValue{ string };
            if (!archive.SerializeKey(stringValue))
                return false;
        }
    }
    return true;
}

/// Serialize vector with standard interface. Content is serialized as separate objects.
template <class T>
inline bool SerializeVectorAsObjects(Archive& archive, const char* name, const char* element, T& vector)
{
    using ValueType = typename T::value_type;
    if (auto block = archive.OpenArrayBlock(name, vector.size()))
    {
        if (archive.IsInput())
        {
            vector.clear();
            vector.resize(block.GetSizeHint());
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                if (!SerializeValue(archive, element, vector[i]))
                    return false;
            }
            return true;
        }
        else
        {
            for (ValueType& value : vector)
            {
                if (!SerializeValue(archive, element, value))
                    return false;
            }
            return true;
        }
    }
    return false;
}

/// Serialize vector with standard interface. Content is serialized as bytes.
template <class T>
inline bool SerializeVectorAsBytes(Archive& archive, const char* name, T& vector)
{
    using ValueType = typename T::value_type;
    static_assert(std::is_standard_layout<ValueType>::value, "Type should have standard layout to safely use byte serialization");
    static_assert(std::is_trivially_copyable<ValueType>::value, "Type should be trivially copyable to safely use byte serialization");

    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        if (archive.IsInput())
        {
            unsigned sizeInBytes{};
            if (!archive.SerializeVLE("size", sizeInBytes))
                return false;

            if (sizeInBytes % sizeof(ValueType) != 0)
            {
                archive.SetError(Format("Unexpected size of byte array '{0}'", name));
                return false;
            }

            vector.resize(sizeInBytes / sizeof(ValueType));
            if (!archive.SerializeBytes("data", vector.data(), sizeInBytes))
                return false;
        }
        else
        {
            unsigned sizeInBytes = vector.size() * sizeof(ValueType);
            if (!archive.SerializeVLE("size", sizeInBytes))
                return false;

            if (!archive.SerializeBytes("data", vector.data(), sizeInBytes))
                return false;
        }
        return true;
    }
    return false;
}

/// Serialize vector in the best possible format.
template <class T>
inline bool SerializeVector(Archive& archive, const char* name, const char* element, T& vector)
{
    using ValueType = typename T::value_type;
    static constexpr bool standardLayout = std::is_standard_layout<ValueType>::value;
    static constexpr bool triviallyCopyable = std::is_trivially_copyable<ValueType>::value;

    if (archive.IsHumanReadable())
        return SerializeVectorAsObjects(archive, name, element, vector);
    else
    {
        if constexpr (standardLayout && triviallyCopyable)
            return SerializeVectorAsBytes(archive, name, vector);
        else
            return SerializeVectorAsObjects(archive, name, element, vector);
    }
}

/// Serialize custom vector.
/// While writing, serializer may skip vector elements. Size should match actual number of elements to be written.
/// While reading, serializer must push elements into vector on its own.
template <class T, class U>
inline bool SerializeCustomVector(Archive& archive, ArchiveBlockType blockType, const char* name, unsigned sizeToWrite, const T& vector, U serializer)
{
    assert(blockType == ArchiveBlockType::Array || blockType == ArchiveBlockType::Map);

    using ValueType = typename T::value_type;
    if (auto block = archive.OpenBlock(name, sizeToWrite, false, blockType))
    {
        if (archive.IsInput())
        {
            for (unsigned index = 0; index < block.GetSizeHint(); ++index)
            {
                static const ValueType valuePlaceholder{};
                if (!serializer(index, valuePlaceholder, true))
                {
                    archive.SetError(Format("Failed to read {0}-th element of container '{1}'", index, name));
                    return false;
                }
            }
            return true;
        }
        else
        {
            for (unsigned index = 0; index < vector.size(); ++index)
            {
                if (!serializer(index, vector[index], false))
                {
                    archive.SetError(Format("Failed to write {0}-th element of container '{1}'", index, name));
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

/// Serialize custom map.
/// While writing, serializer may skip map elements. Size should match actual number of elements to be written.
/// While reading, serializer must push elements into map on its own.
template <class T, class U>
inline bool SerializeCustomMap(Archive& archive, ArchiveBlockType blockType, const char* name, unsigned sizeToWrite, const T& map, U serializer)
{
    assert(blockType == ArchiveBlockType::Array || blockType == ArchiveBlockType::Map);

    using KeyType = typename T::key_type;
    using MappedType = typename T::mapped_type;
    if (auto block = archive.OpenBlock(name, sizeToWrite, false, blockType))
    {
        if (archive.IsInput())
        {
            for (unsigned index = 0; index < block.GetSizeHint(); ++index)
            {
                static const KeyType keyPlaceholder{};
                static const MappedType valuePlaceholder{};
                if (!serializer(index, keyPlaceholder, valuePlaceholder, true))
                {
                    archive.SetError(Format("Failed to read {0}-th element of container '{1}'", index, name));
                    return false;
                }
            }
            return true;
        }
        else
        {
            unsigned index = 0;
            for (auto& elem : map)
            {
                // Copy the key so it could be passed by reference
                if (!serializer(index, elem.first, elem.second, false))
                {
                    archive.SetError(Format("Failed to write {0}-th element of container '{1}'", index, name));
                    return false;
                }
                ++index;
            }
            return true;
        }
    }
    return false;
}

/// Serialize map or hash map with string key with standard interface.
template <class T>
inline bool SerializeStringMap(Archive& archive, const char* name, const char* element, T& map)
{
    using ValueType = typename T::mapped_type;
    if (auto block = archive.OpenMapBlock(name, map.size()))
    {
        if (archive.IsInput())
        {
            map.clear();
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                ea::string key{};
                ValueType value{};
                archive.SerializeKey(key);
                if (!SerializeValue(archive, element, value))
                    return false;
                map.emplace(std::move(key), std::move(value));
            }
            return true;
        }
        else
        {
            for (auto& keyValue : map)
            {
                archive.SerializeKey(const_cast<ea::string&>(keyValue.first));
                SerializeValue(archive, element, keyValue.second);
            }
            return true;
        }
    }
    return false;
}

/// Serialize map or hash map with unsigned integer key with standard interface.
template <class T>
inline bool SerializeStringHashMap(Archive& archive, const char* name, const char* element, T& map)
{
    using ValueType = typename T::mapped_type;
    if (auto block = archive.OpenMapBlock(name, map.size()))
    {
        if (archive.IsInput())
        {
            map.clear();
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                ValueType value{};
                unsigned key{};
                archive.SerializeKey(key);
                if (!SerializeValue(archive, element, value))
                    return false;
                map.emplace(StringHash{ key }, std::move(value));
            }
            return true;
        }
        else
        {
            for (auto& keyValue : map)
            {
                unsigned key = keyValue.first.Value();
                archive.SerializeKey(key);
                SerializeValue(archive, element, keyValue.second);
            }
            return true;
        }
    }
    return false;
}

/// Serialize StringVector.
inline bool SerializeValue(Archive& archive, const char* name, StringVector& value)
{
    return SerializeVectorAsObjects(archive, name, nullptr, value);
}

/// Serialize VariantVector.
inline bool SerializeValue(Archive& archive, const char* name, VariantVector& value)
{
    return SerializeVectorAsObjects(archive, name, nullptr, value);
}

/// Serialize VariantMap.
inline bool SerializeValue(Archive& archive, const char* name, VariantMap& value)
{
    return SerializeStringHashMap(archive, name, nullptr, value);
}

/// Serialize ResourceRef.
inline bool SerializeValue(Archive& archive, const char* name, ResourceRef& value)
{
    if (!archive.IsHumanReadable())
    {
        if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
        {
            bool success = true;
            success &= SerializeValue(archive, "type", value.type_);
            success &= SerializeValue(archive, "name", value.name_);
            return success;
        }
        return false;
    }
    else
    {
        Context* context = archive.GetContext();
        if (!context)
            return false;

        if (archive.IsInput())
        {
            ea::string stringValue;
            if (!SerializeValue(archive, name, stringValue))
                return false;

            ea::vector<ea::string> chunks = stringValue.split(';');
            if (chunks.size() != 2)
            {
                archive.SetError(Format("Unexpected format of ResourceRef '{0}'", name));
                return false;
            }

            value.type_ = chunks[0];
            value.name_ = chunks[1];
        }
        else
        {
            ea::string stringValue = Detail::FormatResourceRefList(context->GetTypeName(value.type_), &value.name_, 1);
            if (!SerializeValue(archive, name, stringValue))
                return false;
        }
        return true;
    }
}

/// Serialize ResourceRefList.
inline bool SerializeValue(Archive& archive, const char* name, ResourceRefList& value)
{
    if (!archive.IsHumanReadable())
    {
        if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
        {
            bool success = true;
            success &= SerializeValue(archive, "type", value.type_);
            success &= SerializeVectorAsObjects(archive, "name", "element", value.names_);
            return success;
        }
        return false;
    }
    else
    {
        Context* context = archive.GetContext();
        if (!context)
            return false;

        if (archive.IsInput())
        {
            ea::string stringValue;
            if (!SerializeValue(archive, name, stringValue))
                return false;

            ea::vector<ea::string> chunks = stringValue.split(';');
            if (chunks.size() < 1)
            {
                archive.SetError(Format("Unexpected format of ResourceRefList '{0}'", name));
                return false;
            }

            value.type_ = chunks[0];
            chunks.pop_front();
            value.names_ = std::move(chunks);
        }
        else
        {
            ea::string stringValue = Detail::FormatResourceRefList(context->GetTypeName(value.type_), value.names_.data(), value.names_.size());
            if (!SerializeValue(archive, name, stringValue))
                return false;
        }
        return true;
    }
}

/// Serialize value of the Variant.
URHO3D_API bool SerializeVariantValue(Archive& archive, VariantType variantType, const char* name, Variant& value);

/// Serialize Variant.
inline bool SerializeValue(Archive& archive, const char* name, Variant& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        VariantType variantType = value.GetType();
        if (!SerializeEnum(archive, "type", Variant::GetTypeNameList(), variantType))
            return false;

        return SerializeVariantValue(archive, variantType, "value", value);
    }
    return false;
}

/// Serialize Object.
template <class T, std::enable_if_t<std::is_base_of<Object, T>::value, int> = 0>
inline bool SerializeValue(Archive& archive, const char* name, SharedPtr<T>& value)
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

/// Serialize optional element or block.
template <class T>
inline bool SerializeOptional(Archive& archive, bool cond, T serializer, bool emulate = true)
{
    const bool loading = archive.IsInput();
    if (archive.IsUnorderedSupportedNow())
    {
        if (loading)
        {
            return serializer(true);
        }
        else
        {
            if (cond)
                return serializer(false);
            return true;
        }
    }
    else if (emulate)
    {
        if (ArchiveBlock block = archive.OpenUnorderedBlock("optional"))
        {
            if (!SerializeValue(archive, "exists", cond))
                return false;

            if (cond)
                return serializer(loading);

            return true;
        }
        return false;
    }
    else
    {
        return serializer(loading);
    }
}

/// Serialize value with custom setter callback.
template <class T, class U>
inline bool SerializeCustomValue(Archive& archive, const char* name, const T& value, U setter)
{
    if (archive.IsInput())
    {
        T tempValue;
        const bool success = SerializeValue(archive, name, tempValue);
        setter(tempValue);
        return success;
    }
    else
        return SerializeValue(archive, name, const_cast<T&>(value));
}

/// Serialize value with custom setter callback.
template <class T, class U>
inline bool SerializeCustomEnum(Archive& archive, const char* name, const char* const* enumConstants, const T& value, U setter)
{
    if (archive.IsInput())
    {
        T tempValue;
        const bool success = SerializeEnum(archive, name, enumConstants, tempValue);
        setter(tempValue);
        return success;
    }
    else
        return SerializeEnum(archive, name, enumConstants, const_cast<T&>(value));
}

}
