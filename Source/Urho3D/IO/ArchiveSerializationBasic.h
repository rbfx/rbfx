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

#include "../Core/TypeTrait.h"
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

#include <type_traits>

namespace Urho3D
{

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

/// Default callback for value serialization: use SerializeValue.
struct DefaultSerializer
{
    template <class T>
    void operator()(Archive& archive, const char* name, T& value) const { SerializeValue(archive, name, value); }
};

/// Default converter: any type to/from any type.
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

template <> struct EnumStringCaster<unsigned>
{
    const char* const* enumConstants_{};

    ea::string ToArchive(Archive& archive, const char* name, const unsigned& value) const
    {
        return enumConstants_[value];
    }

    unsigned FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        return GetStringListIndex(value.c_str(), enumConstants_, 0);
    }
};

template <class T> struct EnumStringSafeCaster
{
    using UnderlyingInteger = std::underlying_type_t<T>;
    ea::span<ea::string_view> enumConstants_{};

    ea::string ToArchive(Archive& archive, const char* name, const T& value) const
    {
        UnderlyingInteger index = static_cast<UnderlyingInteger>(value);
        if (index < 0 || index >= enumConstants_.size())
            return ea::to_string(index);
        return ea::string{enumConstants_[index]};
    }

    T FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        constexpr unsigned invalidIndex = ea::numeric_limits<unsigned>::max();
        unsigned index = GetStringListIndex(value.c_str(), enumConstants_, invalidIndex);
        if (index == invalidIndex)
        {
            char* end;
            const unsigned long res = std::strtoul(value.c_str(), &end, 10);
            index = (res == ULONG_MAX) ? 0 : static_cast<unsigned>(res);
        }
        return static_cast<T>(index);
    }
};

template <> struct EnumStringSafeCaster<unsigned>
{
    ea::span<ea::string_view> enumConstants_{};

    ea::string_view ToArchive(Archive& archive, const char* name, const unsigned& value) const
    {
        if (value >= enumConstants_.size())
            return ea::to_string(value);
        return enumConstants_[value];
    }

    unsigned FromArchive(Archive& archive, const char* name, const ea::string& value) const
    {
        constexpr unsigned invalidIndex = ea::numeric_limits<unsigned>::max();
        unsigned index = GetStringListIndex(value, enumConstants_, invalidIndex);
        if (index == invalidIndex)
        {
            char* end;
            const unsigned long res = std::strtoul(value.c_str(), &end, 10);
            index = (res == ULONG_MAX) ? 0 : static_cast<unsigned>(res);
        }
        return index;
    }
};

}

/// Check whether the object can be serialized from/to Archive block.
URHO3D_TYPE_TRAIT(IsObjectSerializableInBlock, std::declval<T&>().SerializeInBlock(std::declval<Archive&>()));

/// Check whether the object has "empty" method.
URHO3D_TYPE_TRAIT(IsObjectEmptyCheckable, std::declval<T&>().empty());

/// Placeholder that represents any empty object as default value in SerializeOptionalValue.
struct EmptyObject
{
    template <class T>
    bool operator==(const T& rhs) const
    {
        if constexpr (IsObjectEmptyCheckable<T>::value)
            return rhs.empty();
        else
            return rhs == T{};
    }

    template <class T>
    explicit operator T() const { return T{}; }
};

/// Placeholder that doesn't represent any object in SerializeOptionalValue.
struct AlwaysSerialize
{
    template <class T>
    bool operator==(const T& rhs) const { return false; }
};

/// Placeholder object that can be serialized as nothing.
struct EmptySerializableObject
{
    void SerializeInBlock(Archive& archive) {}
};

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
inline void SerializeValue(Archive& archive, const char* name, StringHash& value) { archive.Serialize(name, value.MutableValue()); }
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

/// Serialize object with standard interface as value.
template <class T, std::enable_if_t<IsObjectSerializableInBlock<T>::value, int> = 0>
inline void SerializeValue(Archive& archive, const char* name, T& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    value.SerializeInBlock(archive);
}

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

/// Serialize enum as integer.
template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
inline void SerializeValue(Archive& archive, const char* name, T& value)
{
    using UnderlyingInteger = std::underlying_type_t<T>;
    SerializeValueAsType<UnderlyingInteger>(archive, name, value);
}

/// Serialize string hash as integer or as string.
inline void SerializeStringHash(Archive& archive, const char* name, StringHash& value, const ea::string_view stringHint)
{
    if (!archive.IsHumanReadable())
        SerializeValue(archive, name, value);
    else
        SerializeValueAsType<ea::string>(archive, name, value, Detail::StringHashCaster{stringHint});
}

/// Serialize enum as integer or as string.
template <class EnumType, class UnderlyingInteger = std::underlying_type_t<EnumType>>
void SerializeEnum(Archive& archive, const char* name, EnumType& value, const char* const* enumConstants)
{
    URHO3D_ASSERT(enumConstants);

    if (!archive.IsHumanReadable())
        SerializeValueAsType<UnderlyingInteger>(archive, name, value);
    else
        SerializeValueAsType<ea::string>(archive, name, value, Detail::EnumStringCaster<EnumType>{enumConstants});
}

/// Serialize enum as integer or as string.
template <class EnumType, class UnderlyingInteger = std::underlying_type_t<EnumType>>
void SerializeEnum(Archive& archive, const char* name, EnumType& value, const ea::span<ea::string_view> enumConstants)
{
    if (!archive.IsHumanReadable())
        SerializeValueAsType<UnderlyingInteger>(archive, name, value);
    else
        SerializeValueAsType<ea::string>(
            archive, name, value, Detail::EnumStringSafeCaster<EnumType>{enumConstants});
}

/// Serialize optional element or block.
template <class T, class U = EmptyObject, class TSerializer = Detail::DefaultSerializer>
void SerializeStrictlyOptionalValue(Archive& archive, const char* name, T& value, const U& defaultValue = U{},
    const TSerializer& serializeValue = TSerializer{})
{
    const bool loading = archive.IsInput();

    if (!archive.IsUnorderedAccessSupportedInCurrentBlock())
    {
        ArchiveBlock block = archive.OpenUnorderedBlock(name);

        bool initialized{};

        if (!loading)
            initialized = !(defaultValue == value);

        SerializeValue(archive, "initialized", initialized);

        if (initialized)
            serializeValue(archive, "value", value);
        else if (loading)
            value = static_cast<T>(defaultValue);
    }
    else
    {
        const bool initialized = loading ? archive.HasElementOrBlock(name) : !(defaultValue == value);
        if (initialized)
            serializeValue(archive, name, value);
        else if (loading)
            value = static_cast<T>(defaultValue);
    }
}

/// Serialize element or block that's optional if archive type supports it.
/// There's no overhead on optionality if Archive doesn't support optional blocks.
template <class T, class U = EmptyObject, class TSerializer = Detail::DefaultSerializer>
void SerializeOptionalValue(Archive& archive, const char* name, T& value, const U& defaultValue = U{},
    const TSerializer& serializeValue = TSerializer{})
{
    if (!archive.IsUnorderedAccessSupportedInCurrentBlock())
    {
        serializeValue(archive, name, value);
        return;
    }

    const bool loading = archive.IsInput();
    const bool initialized = loading ? archive.HasElementOrBlock(name) : !(defaultValue == value);
    if (initialized)
        serializeValue(archive, name, value);
    else if (loading)
    {
        // Don't try to cast from AlwaysSerialize
        if constexpr(!std::is_base_of_v<AlwaysSerialize, U>)
            value = static_cast<T>(defaultValue);
    }
}

/// Serialize pair type.
template <class T, class U, class TSerializer = Detail::DefaultSerializer>
void SerializeValue(
    Archive& archive, const char* name, ea::pair<T, U>& value, const TSerializer& serializeValue = TSerializer{})
{
    const ArchiveBlock block = archive.OpenUnorderedBlock(name);
    serializeValue(archive, "first", value.first);
    serializeValue(archive, "second", value.second);
}

/// Wrapper that consumes ArchiveException and converts it to boolean status.
template <class T>
bool ConsumeArchiveException(const T& lambda, bool errorOnException = true)
{
    try
    {
        lambda();
        return true;
    }
    catch (const ArchiveException& e)
    {
        if (errorOnException)
            URHO3D_LOGERROR("Serialization error: {}", e.what());
        else
            URHO3D_LOGDEBUG("Archive cannot be serialization: {}", e.what());
        return false;
    }
}

}
