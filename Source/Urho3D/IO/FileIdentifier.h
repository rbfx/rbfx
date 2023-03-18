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

#include <Urho3D/Urho3D.h>

#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace Urho3D
{

/// File identifier, similar to Uniform Resource Identifier (URI).
/// Known differences:
/// - If URI starts with `/` or `x:/` it is treated as `file` scheme automatically.
/// - Host names are not supported for `file:` scheme.
///   All of `file:/path/to/file`, `file://path/to/file`, and `file:///path/to/file` are supported
///   and denote absolute file path.
/// - If URI does not contain `:`, it is treated as special "empty" scheme,
///   and the entire URI is treated as relative path.
/// - Conversion to URI string uses `scheme://` format.
struct URHO3D_API FileIdentifier
{
    /// File identifier that references nothing.
    static const FileIdentifier Empty;

    /// Construct default.
    FileIdentifier() = default;
    /// Construct from scheme and path (as is).
    FileIdentifier(ea::string_view scheme, ea::string_view fileName);
    /// Deprecated. Use FromUri() instead.
    FileIdentifier(const ea::string& uri) : FileIdentifier(FromUri(uri)) {}

    /// Construct from uri-like path.
    static FileIdentifier FromUri(ea::string_view uri);
    /// Return URI-like path. Does not always return original path.
    ea::string ToUri() const;

    /// Append path to the current path, adding slash in between if it's missing.
    /// Ignores current scheme restrictions.
    void AppendPath(ea::string_view path);

    /// URI-like scheme. May be empty if not specified.
    ea::string scheme_;
    /// URI-like path to the file.
    ea::string fileName_;

    /// Return whether the identifier is empty.
    bool IsEmpty() const { return scheme_.empty() && fileName_.empty(); }

    /// Operators.
    /// @{
    explicit operator bool() const { return !IsEmpty(); }
    bool operator!() const { return IsEmpty(); }

    bool operator<(const FileIdentifier& rhs) const noexcept
    {
        return (scheme_ < rhs.scheme_) || (scheme_ == rhs.scheme_ && fileName_ < rhs.fileName_);
    }

    bool operator==(const FileIdentifier& rhs) const noexcept
    {
        return scheme_ == rhs.scheme_ && fileName_ == rhs.fileName_;
    }

    bool operator!=(const FileIdentifier& rhs) const noexcept { return !(rhs == *this); }

    FileIdentifier& operator+=(ea::string_view rhs)
    {
        AppendPath(rhs);
        return *this;
    }

    FileIdentifier operator+(ea::string_view rhs) const
    {
        FileIdentifier tmp = *this;
        tmp += rhs;
        return tmp;
    }
    /// @}

    static ea::string SanitizeFileName(ea::string_view fileName);
};

} // namespace Urho3D
