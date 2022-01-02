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

#include "../Core/Assert.h"
#include "../Core/Object.h"
#include "../Core/StringUtils.h"
#include "../Core/TypeTrait.h"
#include "../Core/Variant.h"
#include "../IO/Archive.h"
#include "../IO/Log.h"
#include "../Math/Color.h"
#include "../Math/Matrix3.h"
#include "../Math/Matrix3x4.h"
#include "../Math/Matrix4.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"

#include <EASTL/functional.h>
#include <EASTL/string.h>

#include <type_traits>

namespace Urho3D
{

class Resource;

namespace Detail
{

URHO3D_API ea::string NumberArrayToString(float* values, unsigned size);
URHO3D_API ea::string NumberArrayToString(int* values, unsigned size);
URHO3D_API unsigned StringToNumberArray(const ea::string& string, float* values, unsigned maxSize);
URHO3D_API unsigned StringToNumberArray(const ea::string& string, int* values, unsigned maxSize);

/// Serialize primitive array type as bytes or as formatted string.
template <unsigned N, class T>
inline void SerializePrimitiveArray(Archive& archive, const char* name, T& value)
{
    // Serialize as bytes if we don't care about readability
    if (!archive.IsHumanReadable())
    {
        archive.SerializeBytes(name, &value, sizeof(value));
        return;
    }

    // Serialize as string otherwise
    using ElementType = std::decay_t<decltype(*value.Data())>;
    ElementType* elements = const_cast<ElementType*>(value.Data());

    const bool loading = archive.IsInput();
    ea::string string;

    if (!loading)
        string = NumberArrayToString(elements, N);

    archive.Serialize(name, string);

    if (loading)
        StringToNumberArray(string, elements, N);
}

/// Serialize tie of vectors of the same size. Each tie of elements is serialized as separate object.
template <class T, class U, size_t... Is>
inline void SerializeVectorTie(Archive& archive, const char* name, const char* element, T& vectorTuple, const U& serializeValue, ea::index_sequence<Is...>)
{
    const unsigned sizes[] = { ea::get<Is>(vectorTuple).size()... };

    unsigned numElements = sizes[0];
    auto block = archive.OpenArrayBlock(name, numElements);

    if (archive.IsInput())
    {
        numElements = block.GetSizeHint();
        (ea::get<Is>(vectorTuple).clear(), ...);
        (ea::get<Is>(vectorTuple).resize(numElements), ...);
    }
    else
    {
        if (ea::find(ea::begin(sizes), ea::end(sizes), numElements, ea::not_equal_to<unsigned>{}) != ea::end(sizes))
            throw ArchiveException("Vectors of '{}/{}' have mismatching sizes", archive.GetCurrentBlockPath(), name);
    }

    for (unsigned i = 0; i < numElements; ++i)
    {
        const auto elementTuple = ea::tie(ea::get<Is>(vectorTuple)[i]...);
        serializeValue(archive, element, elementTuple);
    }
}

/// Default callback for value serialization.
struct DefaultSerializer
{
    template <class T>
    void operator()(Archive& archive, const char* name, T& value) const { SerializeValue(archive, name, value); }
};

/// Any type to/from any type.
template <class InternalType, class ExternalType>
struct DefaultTypeCaster
{
    InternalType ToArchive(Archive& archive, const char* name, const ExternalType& value) const
    {
        return static_cast<InternalType>(value);
    }

    ExternalType FromArchive(Archive& archive, const char* name, const InternalType& value) const
    {
        return static_cast<ExternalType>(value);
    }
};

/// String hash to/from string.
struct StringHashCaster
{
    ea::string_view stringHint_;

    ea::string ToArchive(Archive& archive, const char* name, const StringHash& value) const
    {
        return ea::string(stringHint_);
    }

    StringHash FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        return static_cast<StringHash>(value);
    }
};

/// Enum to/from string.
template <class T>
struct EnumStringCaster
{
    using UnderlyingInteger = std::underlying_type_t<T>;
    const char* const* enumConstants_{};

    ea::string ToArchive(Archive& archive, const char* name, const T& value) const
    {
        return enumConstants_[static_cast<UnderlyingInteger>(value)];
    }

    T FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        return static_cast<T>(GetStringListIndex(value.c_str(), enumConstants_, 0));
    }
};

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
        const ea::vector<ea::string> chunks = value.split(';');
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
        ea::vector<ea::string> chunks = value.split(';');
        if (chunks.empty())
            throw ArchiveException("Unexpected format of ResourceRefList '{}/{}'", archive.GetCurrentBlockPath(), name);

        const ea::string typeName = ea::move(chunks[0]);
        chunks.pop_front();
        return { StringHash{typeName}, chunks };
    }
};

}

/// Check whether the object can be serialized from/to Archive block.
URHO3D_TYPE_TRAIT(IsObjectSerializableInBlock, std::declval<T&>().SerializeInBlock(std::declval<Archive&>(), std::declval<ArchiveBlock&>()));

/// Serialize object with standard interface as value.
template <class T, std::enable_if_t<IsObjectSerializableInBlock<T>::value, int> = 0>
inline void SerializeValue(Archive& archive, const char* name, T& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    value.SerializeInBlock(archive, block);
}

/// @name Serialize primitive types
/// @{
inline void SerializeValue(Archive& archive, const char* name, bool& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, signed char& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, unsigned char& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, short& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, unsigned short& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, int& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, unsigned int& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, long long& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, unsigned long long& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, float& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, double& value) { archive.Serialize(name, value); }
inline void SerializeValue(Archive& archive, const char* name, ea::string& value) { archive.Serialize(name, value); }
/// @}

/// @name Serialize primitive array types
/// @{
inline void SerializeValue(Archive& archive, const char* name, Vector2& value) { Detail::SerializePrimitiveArray<2>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Vector3& value) { Detail::SerializePrimitiveArray<3>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Vector4& value) { Detail::SerializePrimitiveArray<4>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Matrix3& value) { Detail::SerializePrimitiveArray<9>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Matrix3x4& value) { Detail::SerializePrimitiveArray<12>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Matrix4& value) { Detail::SerializePrimitiveArray<16>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Rect& value) { Detail::SerializePrimitiveArray<4>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Quaternion& value) { Detail::SerializePrimitiveArray<4>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, Color& value) { Detail::SerializePrimitiveArray<4>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, IntVector2& value) { Detail::SerializePrimitiveArray<2>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, IntVector3& value) { Detail::SerializePrimitiveArray<3>(archive, name, value); }
inline void SerializeValue(Archive& archive, const char* name, IntRect& value) { Detail::SerializePrimitiveArray<4>(archive, name, value); }
/// @}

/// Serialize value as another type.
template <class T, class U, class TCaster = Detail::DefaultTypeCaster<T, U>>
void SerializeValueAsType(Archive& archive, const char* name, U& value, const TCaster& caster = TCaster{})
{
    const bool loading = archive.IsInput();
    T convertedValue{};

    if (!loading)
        convertedValue = caster.ToArchive(archive, name, value);

    SerializeValue(archive, name, convertedValue);

    if (loading)
        value = caster.FromArchive(archive, name, convertedValue);
}

/// Serialize StringHash as integer.
inline void SerializeValue(Archive& archive, const char* name, StringHash& value) { SerializeValue(archive, name, value.MutableValue()); }

/// Serialize string hash as integer or as string.
inline void SerializeStringHash(Archive& archive, const char* name, StringHash& value, const ea::string_view stringHint)
{
    if (!archive.IsHumanReadable())
        SerializeValue(archive, name, value);
    else
        SerializeValueAsType<ea::string>(archive, name, value, Detail::StringHashCaster{stringHint});
}

/// Serialize enum as integer as integer or as string.
template <class EnumType, class UnderlyingInteger = std::underlying_type_t<EnumType>>
void SerializeEnum(Archive& archive, const char* name, const char* const* enumConstants, EnumType& value)
{
    URHO3D_ASSERT(enumConstants);

    if (!archive.IsHumanReadable())
        SerializeValueAsType<UnderlyingInteger>(archive, name, value);
    else
        SerializeValueAsType<ea::string>(archive, name, value, Detail::EnumStringCaster<EnumType>{enumConstants});
}

/// Serialize type of the Variant.
inline void SerializeValue(Archive& archive, const char* name, VariantType& value)
{
    SerializeEnum(archive, name, Variant::GetTypeNameList(), value);
}

/// Serialize value of the Variant.
URHO3D_API void SerializeVariantAsType(Archive& archive, VariantType variantType, const char* name, Variant& value);

/// Serialize Variant in existing block.
inline void SerializeVariantInBlock(Archive& archive, ArchiveBlock& block, Variant& value)
{
    VariantType variantType = value.GetType();
    SerializeValue(archive, "type", variantType);
    SerializeVariantAsType(archive, variantType, "value", value);
}

/// Serialize vector with standard interface. Content is serialized as separate objects.
template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeVectorAsObjects(Archive& archive, const char* name, const char* element, T& vector, const TSerializer& serializeValue = TSerializer{})
{
    using ValueType = typename T::value_type;

    unsigned numElements = vector.size();
    auto block = archive.OpenArrayBlock(name, numElements);

    if (archive.IsInput())
    {
        numElements = block.GetSizeHint();
        vector.clear();
        vector.resize(numElements);
    }

    for (unsigned i = 0; i < numElements; ++i)
        serializeValue(archive, element, vector[i]);
}

/// Serialize array with standard interface (compatible with ea::span, ea::array, etc). Content is serialized as separate objects.
template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeArrayAsObjects(Archive& archive, const char* name, const char* element, T& array, const TSerializer& serializeValue = TSerializer{})
{
    const unsigned numElements = array.size();
    auto block = archive.OpenArrayBlock(name, numElements);

    if (archive.IsInput())
    {
        if (numElements != block.GetSizeHint())
            throw ArchiveException("'{}/{}' has unexpected array size", archive.GetCurrentBlockPath(), name);
    }

    for (unsigned i = 0; i < numElements; ++i)
        serializeValue(archive, element, array[i]);
}

template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeVectorTieAsObjects(Archive& archive, const char* name, const char* element, T vectorTuple, const TSerializer& serializeValue = TSerializer{})
{
    static constexpr auto tupleSize = ea::tuple_size_v<T>;
    Detail::SerializeVectorTie(archive, name, element, vectorTuple, serializeValue, ea::make_index_sequence<tupleSize>{});
}

/// Serialize vector with standard interface. Content is serialized as bytes.
template <class T>
void SerializeVectorAsBytes(Archive& archive, const char* name, T& vector)
{
    using ValueType = typename T::value_type;
    static_assert(std::is_standard_layout<ValueType>::value, "Type should have standard layout to safely use byte serialization");
    static_assert(std::is_trivially_copyable<ValueType>::value, "Type should be trivially copyable to safely use byte serialization");

    const bool loading = archive.IsInput();
    ArchiveBlock block = archive.OpenUnorderedBlock(name);

    unsigned sizeInBytes{};

    if (!loading)
        sizeInBytes = vector.size() * sizeof(ValueType);

    archive.SerializeVLE("size", sizeInBytes);

    if (loading)
    {
        if (sizeInBytes % sizeof(ValueType) != 0)
            throw ArchiveException("'{}/{}' has unexpected size in bytes", archive.GetCurrentBlockPath(), name);
        vector.resize(sizeInBytes / sizeof(ValueType));
    }

    archive.SerializeBytes("data", vector.data(), sizeInBytes);
}

/// Serialize vector in the best possible format.
template <class T>
void SerializeVector(Archive& archive, const char* name, const char* element, T& vector)
{
    using ValueType = typename T::value_type;
    static constexpr bool standardLayout = std::is_standard_layout<ValueType>::value;
    static constexpr bool triviallyCopyable = std::is_trivially_copyable<ValueType>::value;

    if constexpr (standardLayout && triviallyCopyable)
    {
        if (!archive.IsHumanReadable())
        {
            SerializeVectorAsBytes(archive, name, vector);
            return;
        }
    }

    SerializeVectorAsObjects(archive, name, element, vector);
}

/// Serialize custom vector.
/// While writing, serializer may skip vector elements. Size should match actual number of elements to be written.
/// While reading, serializer must push elements into vector on its own.
template <class T, class U>
void SerializeCustomVector(Archive& archive, const char* name, unsigned sizeToWrite, const T& vector, U serializer)
{
    using ValueType = typename T::value_type;
    auto block = archive.OpenArrayBlock(name, sizeToWrite);
    if (archive.IsInput())
    {
        for (unsigned index = 0; index < block.GetSizeHint(); ++index)
        {
            static const ValueType valuePlaceholder{};
            serializer(index, valuePlaceholder, true);
        }
    }
    else
    {
        for (unsigned index = 0; index < vector.size(); ++index)
            serializer(index, vector[index], false);
    }
}

/// Serialize map or hash map with with standard interface.
template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeMap(Archive& archive, const char* name, const char* element, T& map, const TSerializer& serializeValue = TSerializer{})
{
    using KeyType = typename T::key_type;
    using ValueType = typename T::mapped_type;

    if constexpr (ea::is_same_v<ValueType, Variant> && ea::is_same_v<TSerializer, Detail::DefaultSerializer>)
    {
        auto block = archive.OpenArrayBlock(name, map.size());
        if (archive.IsInput())
        {
            map.clear();
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                auto elementBlock = archive.OpenUnorderedBlock(element);
                KeyType key{};
                SerializeValue(archive, "key", key);
                SerializeVariantInBlock(archive, elementBlock, map[key]);
            }
        }
        else
        {
            for (const auto& [key, value] : map)
            {
                auto elementBlock = archive.OpenUnorderedBlock(element);
                SerializeValue(archive, "key", const_cast<KeyType&>(key));
                SerializeVariantInBlock(archive, elementBlock, const_cast<Variant&>(value));
            }
        }
    }
    else
    {
        auto block = archive.OpenArrayBlock(name, map.size());
        if (archive.IsInput())
        {
            map.clear();
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                auto elementBlock = archive.OpenUnorderedBlock(element);
                KeyType key{};
                serializeValue(archive, "key", key);
                serializeValue(archive, "value", map[key]);
            }
        }
        else
        {
            for (const auto& [key, value] : map)
            {
                auto elementBlock = archive.OpenUnorderedBlock(element);
                serializeValue(archive, "key", const_cast<KeyType&>(key));
                serializeValue(archive, "value", const_cast<ValueType&>(value));
            }
        }
    }
}

/// Serialize set or hash set with standard interface.
template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeSet(Archive& archive, const char* name, const char* element, T& set, const TSerializer& serializeValue = TSerializer{})
{
    using ValueType = typename T::value_type;

    auto block = archive.OpenArrayBlock(name, set.size());
    if (archive.IsInput())
    {
        set.clear();
        for (unsigned i = 0; i < block.GetSizeHint(); ++i)
        {
            auto elementBlock = archive.OpenUnorderedBlock(element);
            ValueType value{};
            serializeValue(archive, element, value);
            set.emplace(ea::move(value));
        }
    }
    else
    {
        for (const auto& value : set)
            serializeValue(archive, element, const_cast<ValueType&>(value));
    }
}

/// Serialize StringVector.
inline void SerializeValue(Archive& archive, const char* name, StringVector& value)
{
    SerializeVectorAsObjects(archive, name, "value", value);
}

/// Serialize VariantVector.
inline void SerializeValue(Archive& archive, const char* name, VariantVector& value)
{
    SerializeVectorAsObjects(archive, name, "value", value);
}

/// Serialize VariantMap.
inline void SerializeValue(Archive& archive, const char* name, VariantMap& value)
{
    SerializeMap(archive, name, "value", value);
}

/// Serialize ResourceRef.
inline void SerializeValue(Archive& archive, const char* name, ResourceRef& value)
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

/// Serialize ResourceRefList.
inline void SerializeValue(Archive& archive, const char* name, ResourceRefList& value)
{
    if (!archive.IsHumanReadable())
    {
        ArchiveBlock block = archive.OpenUnorderedBlock(name);
        SerializeValue(archive, "type", value.type_);
        SerializeVectorAsObjects(archive, "name", "element", value.names_);
        return;
    }

    SerializeValueAsType<ea::string>(archive, name, value, Detail::ResourceRefListStringCaster{});
}

/// Serialize Variant.
inline void SerializeValue(Archive& archive, const char* name, Variant& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    SerializeVariantInBlock(archive, block, value);
}

/// Serialize Object.
template <class T, std::enable_if_t<std::is_base_of<Object, T>::value, int> = 0>
void SerializeValue(Archive& archive, const char* name, SharedPtr<T>& value)
{
    const bool loading = archive.IsInput();
    ArchiveBlock block = archive.OpenUnorderedBlock(name);

    StringHash type{};
    ea::string_view typeName{};
    if (!loading && value)
    {
        type = value->GetType();
        typeName = value->GetTypeName();
    }

    SerializeStringHash(archive, "type", type, typeName);

    if (loading)
    {
        // Serialize null object
        if (type == StringHash{})
        {
            value = nullptr;
            return;
        }

        // Create instance
        Context* context = archive.GetContext();
        value.StaticCast(context->CreateObject(type));
        if (!value)
        {
            throw ArchiveException("Failed to create object '{}/{}' of type {}", archive.GetCurrentBlockPath(), name,
                type.ToDebugString());
        }
    }

    if (value)
        SerializeValue(archive, "value", *value);
}

/// Serialize optional element or block.
template <class T, class U = T, class TSerializer = Detail::DefaultSerializer>
void SerializeOptionalValue(Archive& archive, const char* name, T& value, const U& defaultValue,
    const TSerializer& serializeValue = TSerializer{})
{
    const bool loading = archive.IsInput();

    if (!archive.IsUnorderedAccessSupportedInCurrentBlock())
    {
        ArchiveBlock block = archive.OpenUnorderedBlock(name);

        bool initialized{};

        if (!loading)
            initialized = value != defaultValue;

        SerializeValue(archive, "initialized", initialized);

        if (initialized)
            SerializeValue(archive, "value", value);
        else if (loading)
            value = defaultValue;
    }
    else
    {
        const bool initialized = loading ? archive.HasElementOrBlock(name) : value != defaultValue;
        if (initialized)
            SerializeValue(archive, name, value);
        else if (loading)
            value = defaultValue;
    }
}

template <class T>
bool ConsumeArchiveException(const T& lambda)
{
    try
    {
        lambda();
        return true;
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Serialization error: {}", e.what());
        return false;
    }
}

}
