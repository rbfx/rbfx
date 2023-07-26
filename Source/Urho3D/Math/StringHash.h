//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "Urho3D/Container/Hash.h"
#include "Urho3D/Math/MathDefs.h"

#include <EASTL/string.h>

namespace Urho3D
{

class StringHashRegister;

/// 32-bit hash value for a string.
class URHO3D_API StringHash
{
public:
    /// Construct with zero value.
    StringHash() noexcept
        : value_(Empty.value_)
    {
    }

    /// Construct with an initial value.
    explicit StringHash(unsigned value) noexcept
        : value_(value)
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
    bool operator==(const StringHash& rhs) const { return value_ == rhs.value_; }

    /// Test for inequality with another hash.
    bool operator!=(const StringHash& rhs) const { return value_ != rhs.value_; }

    /// Test if less than another hash.
    bool operator<(const StringHash& rhs) const { return value_ < rhs.value_; }

    /// Test if greater than another hash.
    bool operator>(const StringHash& rhs) const { return value_ > rhs.value_; }

    /// Return true if nonempty hash value.
    bool IsEmpty() const { return value_ == Empty.value_; }

    /// Return true if nonempty hash value.
    explicit operator bool() const { return !IsEmpty(); }

    /// Return hash value.
    /// @property
    unsigned Value() const { return value_; }

    /// Return mutable hash value. For internal use only.
    unsigned& MutableValue() { return value_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return debug string that contains hash value, and reversed hash string if possible.
    ea::string ToDebugString() const;

    /// Return string which has specific hash value. Return first string if many (in order of calculation).
    /// Use for debug purposes only. Return empty string if URHO3D_HASH_DEBUG is off.
    ea::string Reverse() const;

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return value_; }

    /// Calculate hash value from a C string.
    static unsigned Calculate(const char* str);

    /// Calculate hash value from binary data.
    static unsigned Calculate(const void* data, unsigned length);

    /// Get global StringHashRegister. Use for debug purposes only. Return nullptr if URHO3D_HASH_DEBUG is off.
    static StringHashRegister* GetGlobalStringHashRegister();

    /// Hash of empty string. Is *not* zero!
    static const StringHash Empty;

private:
    /// Hash value.
    unsigned value_;
};

static_assert(sizeof(StringHash) == sizeof(unsigned), "Unexpected StringHash size.");

} // namespace Urho3D
