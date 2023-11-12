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

#include "../IO/ArchiveSerializationBasic.h"
#include "../IO/ArchiveSerializationVariant.h"

namespace Urho3D
{

namespace Detail
{

/// Serialize tie of vectors of the same size. Each tie of elements is serialized as separate block.
template <class T, class U, size_t... Is>
inline void SerializeVectorTie(Archive& archive, const char* name, T& vectorTie, const char* element, const U& serializeValue, ea::index_sequence<Is...>)
{
    const unsigned sizes[] = { ea::get<Is>(vectorTie).size()... };

    unsigned numElements = sizes[0];
    auto block = archive.OpenArrayBlock(name, numElements);

    if (archive.IsInput())
    {
        numElements = block.GetSizeHint();
        (ea::get<Is>(vectorTie).clear(), ...);
        (ea::get<Is>(vectorTie).resize(numElements), ...);
    }
    else
    {
        if (ea::find(ea::begin(sizes), ea::end(sizes), numElements, ea::not_equal_to<unsigned>{}) != ea::end(sizes))
            throw ArchiveException("Vectors of '{}/{}' have mismatching sizes", archive.GetCurrentBlockPath(), name);
    }

    for (unsigned i = 0; i < numElements; ++i)
    {
        const auto elementTuple = ea::tie(ea::get<Is>(vectorTie)[i]...);
        serializeValue(archive, element, elementTuple);
    }
}

URHO3D_TYPE_TRAIT(IsVectorType, (\
    std::declval<T&>().size(),\
    std::declval<T&>().data(),\
    std::declval<T&>().clear(),\
    std::declval<T&>().resize(0u),\
    std::declval<typename T::value_type&>() = std::declval<T&>()[0u]\
));

URHO3D_TYPE_TRAIT(IsMapType, (\
    std::declval<T&>().size(),\
    std::declval<T&>().clear(),\
    std::declval<typename T::mapped_type&>() = std::declval<T&>()[std::declval<typename T::key_type>()]\
));

URHO3D_TYPE_TRAIT(IsSetType, (\
    std::declval<T&>().size(),\
    std::declval<T&>().clear(),\
    std::declval<T&>().emplace(std::declval<typename T::key_type>()),\
    std::declval<typename T::key_type&>() = *std::declval<T&>().begin()\
));

}

/// Serialize vector with standard interface. Content is serialized as separate objects.
template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeVectorAsObjects(Archive& archive, const char* name, T& vector, const char* element = "element",
    const TSerializer& serializeValue = TSerializer{})
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
void SerializeArrayAsObjects(Archive& archive, const char* name, T& array, const char* element = "element",
    const TSerializer& serializeValue = TSerializer{})
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
void SerializeVectorTieAsObjects(Archive& archive, const char* name, T vectorTie, const char* element = "element",
    const TSerializer& serializeValue = TSerializer{})
{
    static constexpr auto tupleSize = ea::tuple_size_v<T>;
    Detail::SerializeVectorTie(archive, name, vectorTie, element, serializeValue, ea::make_index_sequence<tupleSize>{});
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
void SerializeVector(Archive& archive, const char* name, T& vector, const char* element = "element")
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

    SerializeVectorAsObjects(archive, name, vector, element);
}

/// Serialize custom vector.
/// While writing, serializer may skip vector elements. Size should match actual number of elements to be written.
/// While reading, serializer must push elements into vector on its own.
/// TODO: Deprecated. Remove it.
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
void SerializeMap(Archive& archive, const char* name, T& map, const char* element = "element",
    const TSerializer& serializeValue = TSerializer{}, bool clear = true)
{
    using KeyType = typename T::key_type;
    using ValueType = typename T::mapped_type;

    if constexpr (ea::is_same_v<ValueType, Variant> && ea::is_same_v<TSerializer, Detail::DefaultSerializer>)
    {
        auto block = archive.OpenArrayBlock(name, map.size());
        if (archive.IsInput())
        {
            if (clear)
                map.clear();
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                auto elementBlock = archive.OpenUnorderedBlock(element);
                KeyType key{};
                SerializeValue(archive, "key", key);
                SerializeVariantInBlock(archive, map[key]);
            }
        }
        else
        {
            for (const auto& [key, value] : map)
            {
                auto elementBlock = archive.OpenUnorderedBlock(element);
                SerializeValue(archive, "key", const_cast<KeyType&>(key));
                SerializeVariantInBlock(archive, const_cast<Variant&>(value));
            }
        }
    }
    else
    {
        auto block = archive.OpenArrayBlock(name, map.size());
        if (archive.IsInput())
        {
            if (clear)
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
void SerializeSet(Archive& archive, const char* name, T& set, const char* element = "element",
    const TSerializer& serializeValue = TSerializer{})
{
    using ValueType = typename T::value_type;

    auto block = archive.OpenArrayBlock(name, set.size());
    if (archive.IsInput())
    {
        set.clear();
        for (unsigned i = 0; i < block.GetSizeHint(); ++i)
        {
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

/// Serialize shared pointer to Object.
template <class T, std::enable_if_t<std::is_base_of<Object, T>::value, int> = 0>
void SerializeSharedPtr(Archive& archive, const char* name, SharedPtr<T>& value, bool singleBlock = false, bool ignoreUnknown = false)
{
    const bool loading = archive.IsInput();
    ArchiveBlock block = ignoreUnknown ? archive.OpenSafeUnorderedBlock(name) : archive.OpenUnorderedBlock(name);

    StringHash type{};
    ea::string_view typeName{};
    if (!loading && value)
    {
        type = value->GetType();
        typeName = value->GetTypeName();
    }

    SerializeStringHash(archive, singleBlock ? "_Class" : "type", type, typeName);

    if (loading)
    {
        // Serialize null object
        if (type == StringHash::Empty)
        {
            value = nullptr;
            return;
        }

        // Create instance
        Context* context = archive.GetContext();
        value.StaticCast(context->CreateObject(type));
        if (!value)
        {
            if (ignoreUnknown)
            {
                URHO3D_LOGWARNING("Unknown object type is ignored on serialization");
                return;
            }
            else
            {
                throw ArchiveException("Failed to create object '{}/{}' of type {}", archive.GetCurrentBlockPath(), name,
                    type.ToDebugString());
            }
        }
    }

    if (value)
    {
        if (singleBlock)
            value->SerializeInBlock(archive);
        else
            SerializeValue(archive, "value", *value);
    }
}

/// Aliases for SerializeValue.
/// @{
template <class T, std::enable_if_t<Detail::IsVectorType<T>::value, int> = 0>
void SerializeValue(Archive& archive, const char* name, T& vector) { SerializeVector(archive, name, vector); }
template <class T, std::enable_if_t<Detail::IsMapType<T>::value, int> = 0>
void SerializeValue(Archive& archive, const char* name, T& map) { SerializeMap(archive, name, map); }
template <class T, std::enable_if_t<Detail::IsSetType<T>::value, int> = 0>
void SerializeValue(Archive& archive, const char* name, T& set) { SerializeSet(archive, name, set); }
template <class T, std::enable_if_t<std::is_base_of<Object, T>::value, int> = 0>
void SerializeValue(Archive& archive, const char* name, SharedPtr<T>& value) { SerializeSharedPtr(archive, name, value); }
/// @}


// This code is non-intuitive, commented out for now.
#if 0
/// Serialize optional value, construct if empty.
template <class T, class TSerializer = Detail::DefaultSerializer>
void SerializeValue(Archive& archive, const char* name, ea::optional<T>& value, const TSerializer& serializeValue = TSerializer{})
{
    if (!value.has_value())
        value.emplace();
    serializeValue(archive, name, *value);
}
#endif

}
