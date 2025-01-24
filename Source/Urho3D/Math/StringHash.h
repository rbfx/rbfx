// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Hash.h"
#include "Urho3D/Math/MathDefs.h"

#include <EASTL/string.h>

namespace Urho3D
{

namespace Detail
{

/// Calculate hash in compile time.
/// This function should be identical to eastl::hash<string_view> specialization from EASTL/string_view.h
static constexpr size_t CalculateEastlHash(const ea::string_view& x)
{
    using namespace ea;
    string_view::const_iterator p = x.cbegin();
    string_view::const_iterator end = x.cend();
    uint32_t result = 2166136261U; // We implement an FNV-like string hash.
    while (p != end)
        result = (result * 16777619) ^ (uint8_t)*p++;
    return (size_t)result;
}

} // namespace Detail

class StringHashRegister;

/// 32-bit hash value for a string.
class URHO3D_API StringHash
{
public:
    /// Tag to disable population of hash reversal map.
    struct NoReverse
    {
    };

    /// Construct with zero value.
    constexpr StringHash() noexcept
        : value_(EmptyValue)
    {
    }

    /// Construct with an initial value.
    constexpr explicit StringHash(unsigned value) noexcept
        : value_(value)
    {
    }

    /// Construct from a C string (no side effects).
    constexpr StringHash(const char* str, NoReverse) noexcept // NOLINT(google-explicit-constructor)
        : StringHash(ea::string_view{str}, NoReverse{})
    {
    }

    /// Construct from a string (no side effects).
    StringHash(const ea::string& str, NoReverse) noexcept // NOLINT(google-explicit-constructor)
        : StringHash(ea::string_view{str}, NoReverse{})
    {
    }

    /// Construct from a string (no side effects).
    constexpr StringHash(const ea::string_view& str, NoReverse) noexcept // NOLINT(google-explicit-constructor)
        : value_(Calculate(str.data(), static_cast<unsigned>(str.length())))
    {
    }

    /// Construct from a C string.
    StringHash(const char* str) noexcept // NOLINT(google-explicit-constructor)
        : StringHash(ea::string_view{str})
    {
    }

    /// Construct from a string.
    StringHash(const ea::string& str) noexcept // NOLINT(google-explicit-constructor)
        : StringHash(ea::string_view{str})
    {
    }

    /// Construct from a string.
    StringHash(const ea::string_view& str) noexcept; // NOLINT(google-explicit-constructor)

    /// Test for equality with another hash.
    constexpr bool operator==(const StringHash& rhs) const { return value_ == rhs.value_; }

    /// Test for inequality with another hash.
    constexpr bool operator!=(const StringHash& rhs) const { return value_ != rhs.value_; }

    /// Test if less than another hash.
    constexpr bool operator<(const StringHash& rhs) const { return value_ < rhs.value_; }

    /// Test if greater than another hash.
    constexpr bool operator>(const StringHash& rhs) const { return value_ > rhs.value_; }

    /// Return true if nonempty hash value.
    constexpr bool IsEmpty() const { return value_ == EmptyValue; }

    /// Return true if nonempty hash value.
    constexpr explicit operator bool() const { return !IsEmpty(); }

    /// Return hash value.
    /// @property
    constexpr unsigned Value() const { return value_; }

    /// Return mutable hash value. For internal use only.
    constexpr unsigned& MutableValue() { return value_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return debug string that contains hash value, and reversed hash string if possible.
    ea::string ToDebugString() const;

    /// Return string which has specific hash value. Return first string if many (in order of calculation).
    /// Use for debug purposes only. Return empty string if URHO3D_HASH_DEBUG is off.
    ea::string Reverse() const;

    /// Return hash value for HashSet & HashMap.
    constexpr unsigned ToHash() const { return value_; }

    /// Calculate hash value for string_view.
    static constexpr unsigned Calculate(const ea::string_view& view)
    {
        return static_cast<unsigned>(Detail::CalculateEastlHash(view));
    }

    /// Calculate hash value from a C string.
    static constexpr unsigned Calculate(const char* str)
    { //
        return Calculate(ea::string_view(str));
    }

    /// Calculate hash value from binary data.
    static constexpr unsigned Calculate(const char* data, unsigned length)
    {
        return Calculate(ea::string_view(data, length));
    }

    /// Get global StringHashRegister. Use for debug purposes only. Return nullptr if URHO3D_HASH_DEBUG is off.
    static StringHashRegister* GetGlobalStringHashRegister();

    /// Hash of empty string. Is *not* zero!
    static constexpr auto EmptyValue = static_cast<unsigned>(Detail::CalculateEastlHash(ea::string_view{""}));
    static const StringHash Empty;

private:
    /// Hash value.
    unsigned value_;
};

inline constexpr StringHash operator"" _sh(const char* str, std::size_t len)
{
    return StringHash{ea::string_view{str, len}, StringHash::NoReverse{}};
}

static_assert(sizeof(StringHash) == sizeof(unsigned), "Unexpected StringHash size.");

} // namespace Urho3D
