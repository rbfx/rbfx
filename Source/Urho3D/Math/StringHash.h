//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include <EASTL/string.h>

#include "../Container/Hash.h"
#include "../Math/MathDefs.h"

namespace Urho3D
{

class StringHashRegister;

/// 32-bit hash value for a string.
class URHO3D_API StringHash
{
public:
    /// Construct with zero value.
    StringHash() noexcept :
        value_(0)
    {
    }

    /// Copy-construct from another hash.
    StringHash(const StringHash& rhs) noexcept = default;

    /// Construct with an initial value.
    explicit StringHash(unsigned value) noexcept :
        value_(value)
    {
    }

    /// Construct from a C string.
#ifndef URHO3D_HASH_DEBUG
    constexpr StringHash(const char* str) noexcept      // NOLINT(google-explicit-constructor)
        : value_(Calculate(str))
    {
    }
#else
    StringHash(const char* str) noexcept;      // NOLINT(google-explicit-constructor)
#endif
    /// Construct from a string.
    StringHash(const ea::string& str) noexcept;      // NOLINT(google-explicit-constructor)
    /// Construct from a string.
    StringHash(const ea::string_view& str) noexcept;      // NOLINT(google-explicit-constructor)

    /// Assign from another hash.
    StringHash& operator =(const StringHash& rhs) noexcept = default;

    /// Add a hash.
    StringHash operator +(const StringHash& rhs) const
    {
        StringHash ret;
        ret.value_ = value_ + rhs.value_;
        return ret;
    }

    /// Add-assign a hash.
    StringHash& operator +=(const StringHash& rhs)
    {
        value_ += rhs.value_;
        return *this;
    }

    /// Test for equality with another hash.
    bool operator ==(const StringHash& rhs) const { return value_ == rhs.value_; }

    /// Test for inequality with another hash.
    bool operator !=(const StringHash& rhs) const { return value_ != rhs.value_; }

    /// Test if less than another hash.
    bool operator <(const StringHash& rhs) const { return value_ < rhs.value_; }

    /// Test if greater than another hash.
    bool operator >(const StringHash& rhs) const { return value_ > rhs.value_; }

    /// Return true if nonzero hash value.
    explicit operator bool() const { return value_ != 0; }

    /// Return hash value.
    /// @property
    unsigned Value() const { return value_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return string which has specific hash value. Return first string if many (in order of calculation). Use for debug purposes only. Return empty string if URHO3D_HASH_DEBUG is off.
    ea::string Reverse() const;

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return value_; }
#ifndef URHO3D_HASH_DEBUG
    /// Calculate hash value from a C string.
    static constexpr unsigned Calculate(const char* str, unsigned hash = 0)
    {
        return str == nullptr || *str == 0 ? hash : Calculate(str + 1, SDBMHash(hash, (unsigned char)*str));
    }
#else
    /// Calculate hash value from a C string.
    static unsigned Calculate(const char* str, unsigned hash = 0);
#endif
    /// Calculate hash value from binary data.
    static unsigned Calculate(const void* data, unsigned length, unsigned hash = 0);

    /// Get global StringHashRegister. Use for debug purposes only. Return nullptr if URHO3D_HASH_DEBUG is off.
    static StringHashRegister* GetGlobalStringHashRegister();

    /// Zero hash.
    static const StringHash ZERO;

private:
    /// Hash value.
    unsigned value_;
};

static_assert(sizeof(StringHash) == sizeof(unsigned), "Unexpected StringHash size.");

}
