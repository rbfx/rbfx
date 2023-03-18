//
// Copyright (c) 2022-2022 the Urho3D project.
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

namespace Urho3D
{

/// File identifier, similar to Uniform Resource Identifier (URI).
/// Known differences:
/// - Host names are not supported for `file:` scheme.
///   Both `file:/path/to/file` and `file:///path/to/file` are supported and denote absolute file path.
///   URI with `file://host/path/to/file` is not supported and results in error.
/// - If URI does not contain `:`, it is treated as special "empty" scheme,
///   and the entire URI is treated as relative path.
struct URHO3D_API FileIdentifier
{
    /// File identifier that references nothing.
    static const FileIdentifier Empty;

    /// Construct default.
    FileIdentifier() = default;
    /// Construct from scheme and path (as is).
    FileIdentifier(const ea::string& scheme, const ea::string& fileName);
    /// Construct from uri-like path.
    FileIdentifier(const ea::string& url);

    /// URL-like scheme. May be empty if not specified.
    ea::string scheme_;
    /// URL-like path to the file.
    ea::string fileName_;

    /// Return URI-like path.
    ea::string ToUri() const { return scheme_.empty() ? fileName_ : scheme_ + "://" + fileName_; }
    /// Return whether the identifier is empty.
    bool IsEmpty() const { return !scheme_.empty() || !fileName_.empty(); }

    operator bool() const { return !IsEmpty(); }
    bool operator!() const { return IsEmpty(); }

    /// Test for less than with another file locator.
    bool operator<(const FileIdentifier& rhs) const noexcept
    {
        return (scheme_ < rhs.scheme_) || (scheme_ == rhs.scheme_ && fileName_ < rhs.fileName_);
    }

    /// Test for equality with another file locator.
    bool operator==(const FileIdentifier& rhs) const noexcept
    {
        return scheme_ == rhs.scheme_ && fileName_ == rhs.fileName_;
    }

    /// Test for inequality with another file locator.
    bool operator!=(const FileIdentifier& rhs) const noexcept
    {
        return scheme_ != rhs.scheme_ || fileName_ != rhs.fileName_;
    }

    FileIdentifier& operator+=(const ea::string_view& rhs);

    friend FileIdentifier operator+(FileIdentifier lhs, const ea::string& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    friend FileIdentifier operator+(FileIdentifier lhs, const char* rhs)
    {
        lhs += rhs;
        return lhs;
    }

    static ea::string SanitizeFileName(const ea::string& fileName);
};

} // namespace Urho3D
