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

#include "../IO/FileIdentifier.h"

namespace Urho3D
{

FileIdentifier::FileIdentifier()
{
}

FileIdentifier::FileIdentifier(const ea::string& url)
{
    constexpr ea::string_view schemeSeparator(":/");
    auto schemePos = url.find(schemeSeparator);
    if (schemePos == ea::string::npos)
    {
        fileName_ = SanitizeFileName(url);
    }
    else
    {
        scheme_ = url.substr(0, schemePos);
        schemePos += schemeSeparator.size();
        // Skip up to two forward slashes to accommodate common http:// or file:/// url patterns.
        if (schemePos < url.size() && url[schemePos] == '/')
            ++schemePos;
        if (schemePos < url.size() && url[schemePos] == '/')
            ++schemePos;
        fileName_ = SanitizeFileName(url.substr(schemePos));
    }
}

FileIdentifier::FileIdentifier(const ea::string& scheme, const ea::string& fileName)
    : scheme_(scheme)
    , fileName_(SanitizeFileName(fileName))
{
}

ea::string FileIdentifier::SanitizeFileName(const ea::string& fileName)
{
    ea::string sanitizedName = fileName;
    sanitizedName.replace('\\', '/');
    sanitizedName.replace("../", "");
    sanitizedName.replace("./", "");
    sanitizedName.trim();
    return sanitizedName;
}

} // namespace Urho3D
