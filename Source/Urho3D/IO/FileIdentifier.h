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
/// File locator, similar to Universal Resource Locator (URL).
struct URHO3D_API FileIdentifier
{
    /// Construct.
    FileIdentifier();
    /// Construct from url-like path.
    FileIdentifier(const ea::string& url);
    /// Construct from scheme and file name.
    FileIdentifier(const ea::string& scheme, const ea::string& fileName);

    /// URL-like scheme. May be empty if not specified.
    ea::string scheme_;
    /// URL-like path to the file.
    ea::string fileName_;

    operator bool() const { return scheme_.empty() && fileName_.empty(); }
    bool operator!() const { return !(scheme_.empty() && fileName_.empty()); }

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

    static ea::string SanitizeFileName(const ea::string& fileName);
};

} // namespace Urho3D
